#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <linux/if.h>
#include <linux/if_tun.h>

#define BUFFER_SIZE 2048

#define ETH_P_ARP	0x0806
#define ETH_P_IP	0x0800
#define ARP_REQUEST	1
#define ARP_REPLY	2

#define IPPROTO_ICMP 1
#define ICMP_ECHO_REQUEST 8
#define ICMP_ECHO_REPLY	0

static const uint8_t ROUTER_TAP0_MAC[6] = { 0x02, 0x00, 0x00, 0x00, 0x00, 0x01 };
static const uint8_t ROUTER_TAP1_MAC[6] = { 0x02, 0x00, 0x00, 0x00, 0x00, 0x02 };

static uint32_t TAP0_GATEWAY_IP; // 192.168.1.1
static uint32_t TAP1_GATEWAY_IP; // 192.168.2.1

// packed struct definitions
struct __attribute__((packed)) eth_hdr
{
	uint8_t dest_mac[6];
	uint8_t src_mac[6];
	uint16_t ethertype;
};

struct __attribute__((packed)) arp_hdr
{
	uint16_t htype;
	uint16_t ptype;
	uint8_t	hlen;
	uint8_t plen;
	uint16_t opcode;
	uint8_t sender_mac[6];
	uint32_t sender_ip;
	uint8_t target_mac[6];
	uint32_t target_ip;
};

struct __attribute__((packed)) ip_hdr
{
	uint8_t ver_ihl;
	uint8_t tos;
	uint16_t total_len;
	uint16_t id;
	uint16_t flags_offset;
	uint8_t ttl;
	uint8_t protocol;
	uint16_t checksum;
	uint32_t src_ip;
	uint32_t dest_ip;
};

struct __attribute__((packed)) icmp_hdr
{
	uint8_t type;
	uint8_t code;
	uint16_t checksum;
	uint16_t id;
	uint16_t sequence;
};

uint16_t calculate_checksum(void *vdata, size_t length)
{
	char *data = (char*)vdata;
	uint32_t acc = 0;

	for(size_t i = 0; i < length; i += 2)
	{
		uint16_t word;
		if (i + 1 < length)
		{
			memcpy(&word, data + i, 2);
		} else {
			word = 0;
			memcpy(&word, data + i, 1);
		}
		
		acc += ntohs(word);	
	}

	while (acc >> 16) acc = (acc & 0xFFFF) + (acc >> 16);
	return (htons(~acc));
}

int tap_alloc(char *dev_name)
{
	struct ifreq ifr;
	int fd, err;

	if ((fd = open("/dev/net/tun", O_RDWR)) < 0)
	{
		perror("Error opening /dev/net/tun");
		return fd;
	}

	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_flags = IFF_TAP | IFF_NO_PI;

	if (dev_name && *dev_name) strncpy(ifr.ifr_name, dev_name, IFNAMSIZ - 1);

	if ((err = ioctl(fd, TUNSETIFF, (void *)&ifr)) < 0)
	{
		perror("ioctl(TUNSETIFF) failed");
		close(fd);
		return err;
	}

	return fd;
}

void handle_arp(int tap_fd, unsigned char *buffer, ssize_t len, const uint8_t *router_mac, uint32_t gateway_ip)
{
	struct eth_hdr *eth = (struct eth_hdr *)buffer;
	struct arp_hdr *arp = (struct arp_hdr *)(buffer + sizeof(struct eth_hdr));

	if (ntohs(arp->opcode) == ARP_REQUEST && arp->target_ip == gateway_ip)
	{
		// update ethernet header
		memcpy(eth->dest_mac, eth->src_mac, 6);
		memcpy(eth->src_mac, router_mac, 6);

		// update ARP payload
		arp->opcode = htons(ARP_REPLY);
		uint32_t original_sender = arp->sender_ip;
		arp->target_ip = original_sender;
		arp->sender_ip = gateway_ip;
		memcpy(arp->target_mac, arp->sender_mac, 6);
		memcpy(arp->sender_mac, router_mac, 6);

		write(tap_fd, buffer, len);
	}
}

void handle_ip(int in_fd, int out_fd, unsigned char *buffer, ssize_t len, const uint8_t *in_mac, const uint8_t *out_mac)
{
	struct eth_hdr *eth = (struct eth_hdr *)buffer;
	struct ip_hdr *ip = (struct ip_hdr *)(buffer + sizeof(struct eth_hdr));

	int ip_hdr_len = (ip->ver_ihl & 0x0F) *4;

	// check if packet is an ICMP echo request for our router gateway
	if (ip->protocol == IPPROTO_ICMP && (ip->dest_ip == TAP0_GATEWAY_IP || ip->dest_ip == TAP1_GATEWAY_IP))
	{
		struct icmp_hdr *icmp = (struct icmp_hdr *)(buffer + sizeof(struct eth_hdr) + ip_hdr_len);

		if (icmp->type == ICMP_ECHO_REQUEST) {
			char src_str[INET_ADDRSTRLEN], dest_str[INET_ADDRSTRLEN];
			inet_ntop(AF_INET, &ip->src_ip, src_str, INET_ADDRSTRLEN);
			inet_ntop(AF_INET, &ip->dest_ip, dest_str, INET_ADDRSTRLEN);

			printf("	[IMCP_ECHO_REQUEST] Ping from %s -> Gateway %s\n", src_str, dest_str);

			// construct ICMP Reply in-place
			memcpy(eth->dest_mac, eth->src_mac, 6);
			memcpy(eth->src_mac, in_mac, 6);

			// swap ip source & destination
			uint32_t tmp_ip = ip->src_ip;
			ip->src_ip = ip->dest_ip;
			ip->dest_ip = tmp_ip;

			ip->checksum = 0;
			ip->checksum = calculate_checksum(ip, ip_hdr_len);

			// recalculate ip header checksum
			ip->checksum = 0;
			ip->checksum = calculate_checksum(ip, ip_hdr_len);
			
			// convert ICMP request (8) to ICMP Reply (0)
			icmp->type = ICMP_ECHO_REPLY;
			
			// recalculate ICMP payload checksum
			icmp->checksum = 0;
			size_t icmp_len = ntohs(ip->total_len) - ip_hdr_len;
			icmp->checksum = calculate_checksum(icmp, icmp_len);

			// send reply back over the receiving interface
			write(in_fd, buffer, len);
			printf("	[ICMP Echo Reply Sent] Replied to %s\n", src_str);
			return;
		}
	}

	// FORWARDING ENGINE: route packets between interfaces
	if (ip->ttl <= 1)
	{
		printf("	[TTL Expired] Dropping packet\n");
		return;
	}

	ip->ttl--; // decrement TTL

	// recalculate ip checksum after modifying TTL
	ip->checksum = 0;
	ip->checksum = calculate_checksum(ip, ip_hdr_len);

	// update layer 2 ethernet header for forwarding
	memcpy(eth->src_mac, out_mac, 6);
	memset(eth->dest_mac, 0xFF, 6);

	// write packet out to the other interface
	ssize_t written = write(out_fd, buffer, len);
	if (written > 0)
	{
		char src_str[INET_ADDRSTRLEN], dest_str[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &ip->src_ip, src_str, INET_ADDRSTRLEN);
		inet_ntop(AF_INET, &ip->dest_ip, dest_str, INET_ADDRSTRLEN);

		printf("	[Forwared IP Packet] %s -> %s (TTL decremented to %d)\n", src_str, dest_str, ip->ttl);
	}
}

int main(void)
{
	char tap0_name[IFNAMSIZ] = "tap0";
	char tap1_name[IFNAMSIZ] = "tap1";

	inet_pton(AF_INET, "192.168.1.1", &TAP0_GATEWAY_IP);
	inet_pton(AF_INET, "192.168.2.1", &TAP1_GATEWAY_IP);

	int tap0_fd = tap_alloc(tap0_name);
	int tap1_fd = tap_alloc(tap1_name);

	if (tap0_fd < 0 || tap1_fd < 0) return 1;
	
	printf("[+] Router Engine Fully Online!\n");

	unsigned char buffer[BUFFER_SIZE];
	int max_fd = (tap0_fd > tap1_fd) ? tap0_fd : tap1_fd;

	while (1)
	{
		fd_set rd_set;
		FD_ZERO(&rd_set);
		FD_SET(tap0_fd, &rd_set);
		FD_SET(tap1_fd, &rd_set);

		if (select(max_fd + 1, &rd_set, NULL, NULL, NULL) < 0) break;

		// tap0 traffic
		if (FD_ISSET(tap0_fd, &rd_set))
		{
			ssize_t nread = read(tap0_fd, buffer, sizeof(buffer));

			if (nread >= (ssize_t)sizeof(struct eth_hdr))
			{
				struct eth_hdr *eth = (struct eth_hdr *)buffer;
				uint16_t type = ntohs(eth->ethertype);

				if (type == ETH_P_ARP) handle_arp(tap0_fd, buffer, nread, ROUTER_TAP0_MAC, TAP0_GATEWAY_IP);
				else if (type == ETH_P_IP) handle_ip(tap0_fd, tap1_fd, buffer, nread, ROUTER_TAP0_MAC, ROUTER_TAP1_MAC);
			}
		}

		// tap1 traffic
		if (FD_ISSET(tap1_fd, &rd_set))
		{
			ssize_t nread = read(tap1_fd, buffer, sizeof(buffer));
			
			if (nread >= (ssize_t)sizeof(struct eth_hdr))
			{
				struct eth_hdr *eth = (struct eth_hdr *)buffer;
				uint16_t type = ntohs(eth->ethertype);

				if (type == ETH_P_ARP) handle_arp(tap1_fd, buffer, nread, ROUTER_TAP1_MAC, TAP1_GATEWAY_IP);
				else if (type == ETH_P_IP) handle_ip(tap1_fd, tap0_fd, buffer, nread, ROUTER_TAP1_MAC, ROUTER_TAP0_MAC);
			}
		}
	}

	close(tap0_fd);
	close(tap1_fd);

	return 0;
}





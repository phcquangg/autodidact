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

// standard networking constants
#define ETH_P_ARP	0x0806
#define ETH_P_IP	0x0800
#define ARP_REQUEST 1
#define ARP_REPLY	2

// config Router Virtual Interfaces IP & MAC
static const uint8_t ROUTER_TAP0_MAC[6] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x01};
static const uint8_t ROUTER_TAP1_MAC[6] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x02};

// IP Addresses stored in Network Byte Order (Big Endian)
static uint32_t TAP0_GATEWAY_IP; // 192.168.1.1
static uint32_t TAP1_GATEWAY_IP; // 192.168.2.1

// packed structures for header overlay
struct __attribute__((packed)) eth_hdr
{
	uint8_t dest_mac[6];
	uint8_t src_mac[6];
	uint16_t ethertype;
};

struct __attribute__((packed)) arp_hdr
{
	uint16_t	htype;
	uint16_t	ptype;
	uint8_t		hlen;
	uint8_t		plen;
	uint16_t	opcode;
	uint8_t		sender_mac[6];
	uint32_t	sender_ip;
	uint8_t		target_mac[6];
	uint32_t	target_ip;
};

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
	if ((err = ioctl(fd, TUNSETIFF, (void *)&ifr)) < 0) {
		perror("ioctl(TUNSETIFF) failed");
		close(fd);
		return err;
	}
	
	return fd;
}

void handle_arp(int tap_fd, unsigned char *buffer, ssize_t len, const uint8_t *router_mac, uint32_t gateway_ip)
{
	if (len < (ssize_t)(sizeof(struct eth_hdr) + sizeof(struct arp_hdr))) return;

	struct eth_hdr *eth = (struct eth_hdr *)buffer;
	struct arp_hdr *arp = (struct arp_hdr *)(buffer + sizeof(struct eth_hdr));

	// verify ARP opcode is request (1) and target ip matches our gateway ip
	if (ntohs(arp->opcode) == ARP_REQUEST && arp->target_ip == gateway_ip)
	{
		char src_ip_str[INET_ADDRSTRLEN], target_ip_str[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &arp->sender_ip, src_ip_str, INET_ADDRSTRLEN);
		inet_ntop(AF_INET, &arp->target_ip, target_ip_str, INET_ADDRSTRLEN);

		printf("	[ARP Request] Asking for %s | Requested by %s\n", target_ip_str, src_ip_str);

		/* --- Contstruct app reply frame in-place --- */
		
		// update ethernet header
		memcpy(eth->dest_mac, eth->src_mac, 6); // send back to requester
		memcpy(eth->src_mac, router_mac, 6);	// from our router's mac

		// update arp header
		arp->opcode = htons(ARP_REPLY);
		
		// swap ips: target becomes old sender, sender becomes gateway ip
		uint32_t original_sender_ip = arp->sender_ip;
		arp->target_ip = original_sender_ip;
		arp->sender_ip = gateway_ip;

		// swap MAC
		memcpy(arp->target_mac, arp->sender_mac, 6);
		memcpy(arp->sender_mac, router_mac, 6);

		// transmit the ARP reply frame back onto the TAP device
		ssize_t written = write(tap_fd, buffer, len);
		if (written > 0)
		{
			printf("  [ARP Reply Sent] Told %s that %s is at %02x:%02x:%02x:%02x:%02x:%02x\n",
                   src_ip_str, target_ip_str,
                   router_mac[0], router_mac[1], router_mac[2],
                   router_mac[3], router_mac[4], router_mac[5]);
		}
	}
}

int main()
{
	char tap0_name[IFNAMSIZ] = "tap0";
	char tap1_name[IFNAMSIZ] = "tap1";

	// parse gateway ips into network byte order
	inet_pton(AF_INET, "192.168.1.1", &TAP0_GATEWAY_IP);
	inet_pton(AF_INET, "192.168.2.1", &TAP1_GATEWAY_IP);

	int tap0_fd = tap_alloc(tap0_name);
	int tap1_fd = tap_alloc(tap1_name);

	if (tap0_fd < 0 || tap1_fd < 0)
	{
		fprintf(stderr,"Failed to open TAP devices.\n");
		return 1;
	}
	
	printf("[+] Router Engine Online. \n");
	printf("	tap0 Gateway: 192.168.1.1 MAC: 02:00:00:00:00:01\n");
	printf("	tap1 Gateway: 192.168.2.1 MAC: 02:00:00:00:00:02\n\n");

	unsigned char buffer[BUFFER_SIZE];
	int max_fd = (tap0_fd > tap1_fd) ? tap0_fd : tap1_fd;

	while (1)
	{
		fd_set rd_set;
		FD_ZERO(&rd_set);
		FD_SET(tap0_fd, &rd_set);
		FD_SET(tap1_fd, &rd_set);

		if (select(max_fd + 1, &rd_set, NULL, NULL, NULL) < 0)
		{
			perror("select() error");
			break;
		}

		// handle tap0 traffic
		if (FD_ISSET(tap0_fd, &rd_set))
		{
			ssize_t nread = read(tap0_fd, buffer, sizeof(buffer));
			if (nread >= (ssize_t)sizeof(struct eth_hdr))
			{
				struct eth_hdr *eth = (struct eth_hdr *)buffer;
				if (ntohs(eth->ethertype) == ETH_P_ARP)
				{
					printf("[tap0] Received ARP frame (%zd bytes)\n", nread);
					handle_arp(tap0_fd, buffer, nread, ROUTER_TAP0_MAC, TAP0_GATEWAY_IP);
				}
			}
		}

		if (FD_ISSET(tap1_fd, &rd_set))
		{
			ssize_t nread = read(tap1_fd, buffer, sizeof(buffer));
			if (nread >= (ssize_t)sizeof(struct eth_hdr))
			{
				struct eth_hdr *eth = (struct eth_hdr *)buffer;
				if (ntohs(eth->ethertype) == ETH_P_ARP)
				{
					printf("[tap1] Received ARP frame (%zd bytes)\n", nread);
					handle_arp(tap1_fd, buffer, nread, ROUTER_TAP1_MAC, TAP1_GATEWAY_IP);
				}
			}
		}
	}

	close(tap0_fd);
	close(tap1_fd);
	return 0;
}



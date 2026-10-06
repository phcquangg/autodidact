#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <net/ethernet.h>
#include <netinet/if_ether.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <sys/socket.h>

#define BUFFER_SIZE 65536

void process_packet(unsigned char *buffer, int size)
{
	struct ethhdr *eth = (struct ethhdr *)buffer;
	
	printf("\n[Ethernet Frame]\n");
	printf("Source MAC: %02x:%02x:%02x:%02x:%02x:%02x -> Dest MAC: %02x:%02x:%02x:%02x:%02x:%02x | Protocol: 0x%4x\n", eth->h_source[0], eth->h_source[1], eth->h_source[2], eth->h_source[3],eth->h_source[4],eth->h_source[5],eth->h_dest[0],eth->h_dest[1],eth->h_dest[2],eth->h_dest[3],eth->h_dest[4],eth->h_dest[5],ntohs(eth->h_proto));

	if (ntohs(eth->h_proto) == ETH_P_IP)
	{
		struct iphdr *ip = (struct iphdr *)(buffer + sizeof(struct ethhdr));

		struct in_addr src_addr, dest_addr;
		src_addr.s_addr = ip->saddr;
		dest_addr.s_addr = ip->daddr;

		printf("[IPv4 Packet]\n");
		printf("Source IP: %s -> Dest IP: %s | Protocol: %d | TTL: %d\n", inet_ntoa(src_addr), inet_ntoa(dest_addr), ip->protocol, ip->ttl);

		if (ip->protocol == IPPROTO_TCP)
		{
			int ip_header_len = ip->ihl*4;
			
			struct tcphdr *tcp = (struct tcphdr *)(buffer + sizeof(struct ethhdr) + ip_header_len);
			printf("[TCP Segment]\n");
			printf("Source Port: %d -> Dest Port: %d\n", ntohs(tcp->source), ntohs(tcp->dest));
			printf("Sequence: %u | ACK: %u\n", ntohl(tcp->seq), ntohl(tcp->ack_seq));
			printf("Flags -> SYN: %d | ACK: %d | FIN %d\n", tcp->syn, tcp->ack, tcp->fin);
		}
	}
}

int main()
{
	int raw_sock;
	unsigned char buffer[BUFFER_SIZE];

	raw_sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if (raw_sock < 0)
	{
		perror("Socket createion error");
		return 1;
	}

	printf("Listening for raw ethernet frames... \n");

	while (1)
	{
		ssize_t data_size = recvfrom(raw_sock, buffer, BUFFER_SIZE, 0, NULL, NULL);
		if (data_size < 0)
		{
			perror("Recv Error");
			return 1;
		}
		
		process_packet(buffer, data_size);
	}

	close(raw_sock);
	return 0;
}

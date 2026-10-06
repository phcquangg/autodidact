#include <unistd.h>
#include <netinet/tcp.h>
#include <linux/ip.h>
#include <linux/if_ether.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <sys/socket.h>
#include <errno.h>
#include <net/ethernet.h>

#define BUFFER 65536

void tcp_handler(ssize_t packet, unsigned char *buffer)
{
	struct ethhdr *ethernet_hdr = (struct ethhdr *)buffer;

	printf("[Ethernet]\n");
	printf("[MAC] %02x:%02x:%02x:%02x:%02x:%02x --> %02x:%02x:%02x:%02x:%02x:%02x\n", ethernet_hdr->h_source[0], ethernet_hdr->h_source[1], ethernet_hdr->h_source[2], ethernet_hdr->h_source[3], ethernet_hdr->h_source[4], ethernet_hdr->h_source[5], ethernet_hdr->h_dest[0], ethernet_hdr->h_dest[1], ethernet_hdr->h_dest[2], ethernet_hdr->h_dest[3], ethernet_hdr->h_dest[4], ethernet_hdr->h_dest[5]);

	// IPv4 Packets w ETH_P_IP
	if (ntohs(ethernet_hdr->h_proto) == ETH_P_IP) {
		struct iphdr *ip_header = (struct iphdr *)(buffer + sizeof(struct ethhdr));
		
		struct in_addr src_addr, dest_addr;
		src_addr.s_addr = ip_header->saddr;
		dest_addr.s_addr = ip_header->daddr;

		printf("[IPv4] IP: %s -> %s\n", inet_ntoa(src_addr), inet_ntoa(dest_addr));

		if (ip_header->protocol == IPPROTO_TCP) {
			int ip_hdr_len = ip_header->ihl*4;
			struct tcphdr *tcp_header = (struct tcphdr *)(buffer + sizeof(struct ethhdr) + ip_hdr_len);

			printf("[TCP Segment]");
			printf("Port: %d -> %d\n", ntohs(tcp_header->source), ntohs(tcp_header->dest));
			printf("Sequence: %u | ACK: %u\n", ntohl(tcp_header->seq), ntohl(tcp_header->ack_seq));
			printf("SYN: %d | ACK: %d | FIN: %d\n", tcp_header->syn, tcp_header->ack, tcp_header->fin);
		}
	}
};

int main()
{
	unsigned char buffer[BUFFER];
	int tcp_socket;
	
	tcp_socket = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if (tcp_socket < 0) {
		perror("TCP socket: failed to create");
		return 1;
	}

	while (1) {
		ssize_t tcp_packet = recvfrom(tcp_socket, buffer, BUFFER, 0, NULL, NULL);	

		if (tcp_packet < 0) {
			perror("recvfrom failed!");
			return 1;
		}
		
		tcp_handler(tcp_packet, buffer);
	}	
	close(tcp_socket);

	return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define PORT 4444			//	the port client will be connecting to
#define MAX_DATA_SIZE 100	// max number of bytes we can get at once

int main (int argc, char **argv)
{
	int sockfd, numbytes;
	char buf[MAX_DATA_SIZE];
	struct hostent *he;
	struct sockaddr_in their_addr;

	if (argc != 2) {
		fprintf(stderr, "usage: client hostname\n");
		exit(1);
	}

	if ((he = gethostbyname(argv[1])) == NULL) {
		perror("gethostbyname");
		exit(1);
	}

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}

	their_addr.sin_family = AF_INET;
	their_addr.sin_port = htons(PORT);
	their_addr.sin_addr = *((struct in_addr *)he->h_addr);
	memset(&(their_addr.sin_zero), '\0', 8);

	if (connect(sockfd, (struct sockaddr *)&their_addr, sizeof (struct sockaddr)) == -1) {
		perror("connect");	
		exit(1);
	}

	if ((numbytes=recv(sockfd, buf, MAX_DATA_SIZE - 1, 0)) == -1) {
		perror("recv");
		exit(1);
	}

	buf[numbytes] = '\0';
	printf("Received: %s", buf);
	close(sockfd);

	return 0;
}


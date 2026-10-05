#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>

#define READ_BUFFER 50000

int main(int argc, char *argv[])
{
	int sockfd, portno, n;
	struct sockaddr_in serv_addr;
	in_addr_t addr;
	struct hostent *server;
	char buffer[READ_BUFFER];

	if (argc < 3) {
		fprintf(stderr, "usage %s hostname port\n", argv[0]);
		exit(0);
	}

	portno = atoi(argv[2]);
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		perror("ERR opening socket");
		exit(2);
	}

	server = gethostbyname(argv[1]);
	if (server == NULL) {
		fprintf(stderr, "ERR no %d found\n", argv[1]);	
		exit(0);
	}

	bzero((char *)&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(portno);
	if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
		perror("ERR connecting");
		exit(3);
	}

	while (1) {
		printf("Please enter a message: \n");
	
		fgets(buffer, READ_BUFFER -1, stdin);
	
		n = write(sockfd, buffer, strlen(buffer));
		if (n < 0) perror("ERR writing to socket");

		n = read(sockfd, buffer, READ_BUFFER -1);
		if (n < 0) perror("ERR reading from socket");
		else {
			buffer[n] = '\0';
			printf("%s\n", buffer);
		} 
	}

	return 0;
}

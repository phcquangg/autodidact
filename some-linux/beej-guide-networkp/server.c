#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define PORT 4444
#define BACKLOG 10

void sigchld_handler(int s)
{
	while (wait(NULL) > 0);
}

int main(void)
{
	int sockfd, new_fd;		// listen on sock_fd, new connection on new_fd
	struct sockaddr_in my_addr;
	struct sockaddr_in connector_addr;
	int sin_size;
	struct sigaction sa;
	int yes = 1;

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}

	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
		perror("setsockopt");
		exit(1);
	}

	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(PORT);
	my_addr.sin_addr.s_addr = INADDR_ANY;
	memset(&(my_addr.sin_zero), '\0', 8);

	if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1) {
		perror("bind");
		exit(1);
	}

	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}
	
	sa.sa_handler = sigchld_handler; // reap all dead process
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;

	while (1) {
		sin_size = sizeof( struct sockaddr_in);
		if ((new_fd = accept(sockfd, (struct sockaddr *)&connector_addr, &sin_size)) == -1) {
			perror("accept");
			continue;
		}

		printf("server: got connection from %s\n", inet_ntoa(connector_addr.sin_addr));

		if (!fork()) {
			close(sockfd);
			if (send(new_fd, "Hello, world!\n", 14, 0) == -1) perror("send");
			close(new_fd);
			exit(0);
		}

		close(new_fd);
	}

	return 0;
}

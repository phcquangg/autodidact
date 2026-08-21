#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 9043

void exit_err(char *msg)
{
	perror(msg);
	exit(1);
}

int main(void)
{
	fd_set master;
	fd_set read_fds;
	struct sockaddr_in myaddr;
	struct sockaddr_in remoteaddr;
	int fdmax;
	int listener;
	int newfd;
	char buf[256];
	int nbytes;
	int yes = 1;
	int addrlen;
	int i, j;

	FD_ZERO(&master);
	FD_ZERO(&read_fds);

	// get the listener
	if ((listener = socket(AF_INET, SOCK_STREAM, 0)) == -1) exit_err("socket");
	
	// address already in use
	if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) exit_err("setsockopt");

	// bind
	myaddr.sin_family = AF_INET;
	myaddr.sin_port = PORT;
	myaddr.sin_addr.s_addr = INADDR_ANY;
	memset(&(myaddr.sin_zero), '\0', 8);

	if (bind(listener, (struct sockaddr *)&myaddr, sizeof(myaddr)) == -1) exit_err("bind");
	if (listen(listener, 10) == -1) exit_err("listen");

	// add the listener to the master set
	FD_SET(listener, &master);

	// keep track of the biggest fd	
	fdmax = listener;

	for (;;) {
		read_fds = master;
	
		if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) exit_err("select");

		for (i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &read_fds)) {
				if (i == listener) {
					addrlen = sizeof(remoteaddr);
					if ((newfd = accept(listener, (struct sockaddr *)&remoteaddr, &addrlen)) == -1) {
						perror("accept");
					} else {
						FD_SET(newfd, &master);
						if (newfd > fdmax) fdmax = newfd;
						printf("select-server: new connection from %s on '\ socket %d\n", inet_ntoa(remoteaddr.sin_addr), newfd);
					}
				} else {
					if ((nbytes = recv(i, buf, sizeof(buf), 0)) <=0) {
						// got error or connection closed by client 
						printf("select-server: socket %d hung up \n", i);
					} else {
						perror("recv");
					}

					close(i);
					FD_CLR(i, &master); // remove from master set
				}
			} else {
				// we got some data from a client
				for (j = 0; j <= fdmax; j++) {
					// send to everyone!
					if (FD_ISSET(j, &master)) {
						// except the listener and ourselves
						if (j != listener && j != i) {
							if (send(j, buf, nbytes, 0) == -1) {
								perror("send");
							}
						}
					}					
				}
			}
		}
	}

	return 0;
}

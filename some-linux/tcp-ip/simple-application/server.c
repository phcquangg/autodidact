#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>

#define READ_BUFFER 50000

int main(int argc, char *argv[])
{
    int sockfd, newsockfd, port, childpid;
    socklen_t clilen;
    char buffer[READ_BUFFER];
    struct sockaddr_in serv_addr, cli_addr;
    int n;

    // Prevent zombie child processes
    signal(SIGCHLD, SIG_IGN);

    if (argc < 2) {
        fprintf(stderr, "ERR: no port provided\n");
        exit(1);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("ERROR opening socket");
        exit(1);
    }

    bzero((char *)&serv_addr, sizeof(serv_addr));
    port = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERR on binding");
        exit(2);
    }

    listen(sockfd, 5);
    printf("Server listening on port %d...\n", port);

    while (1) {
        clilen = sizeof(cli_addr);
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);

        if (newsockfd < 0) {
            perror("ERR on accept");
            continue; // Keep listening for other incoming connections
        }

        // Fork child process ONLY after a successful accept
        childpid = fork();
        if (childpid < 0) {
            perror("ERR on fork");
            close(newsockfd);
        } else if (childpid == 0) {
            // --- CHILD PROCESS ---
            close(sockfd); // Child doesn't need the listening socket

            while (1) {
                bzero(buffer, READ_BUFFER);
                
                // Read from newsockfd, NOT sockfd
                n = read(newsockfd, buffer, READ_BUFFER - 1);
                if (n <= 0) {
                    if (n < 0) perror("ERR reading from socket");
                    else printf("Client disconnected.\n");
                    break;
                }
                
                // PRINT to server terminal:
                printf("Client sent: %s", buffer);
                
                // Echo back to client using newsockfd
                n = write(newsockfd, buffer, strlen(buffer));
                if (n < 0) {
                    perror("ERR writing to socket");
                    break;
                }
            }

            close(newsockfd);
            exit(0); // Exit child process when client disconnects
        }

        // --- PARENT PROCESS ---
        close(newsockfd); // Parent closes its copy of newsockfd and keeps listening
    }

    close(sockfd);
    return 0;
}

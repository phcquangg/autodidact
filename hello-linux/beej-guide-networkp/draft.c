// socket address information for many types of sockets
struct sockaddr {
	unsigned short	sa_family;		// address family, AF_xxx
	char			sa_data[14];	// 14 bytes for address
};

/*
*	"in" as in Internet
*	sockaddr_in is a parallel structure to sockaddr
*	
*	we can cast sockadd_in into sockaddr and put it into socket();
*		- sin_family & sa_family are corresponding and should be set to AF_xxx
*		- sin_port & sin_addr must be in Network Byte Order (endianess)
*/
struct sockaddr_in {
	short int			sin_family;		// address family
	unsigned short int	sin_port;		// port number	
	struct in_addr		sin_addr;		// internet address
	unsigned char		sin_zero[8];	// should be set to 0 with memset();
};


/*
*	Internet address
*	then we have something like: in_addr.s_addr.s_addr references to the 4 bytes IP address (in NBO)
*/
struct in_addr {
	uint32_t s_addr; // 32-bits or 4 bytes
};

sockaddr_in ina;
ina.sin_addr.s_addr = inet_addr("10.12.110.57"); // inet_addr() returns the address in NBO already without htonl()

/*
*	ascii to network - inet_aton();
*	return non-zero on success and zero on failure, for error-checking.
*/

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int inet_aton(const char *cp, struct in_addr *inp);
struct sockaddr_in my_addr;

my_addr.sin_family = AF_INET; // host byte order
my_addr.sin_port = htons(MYPORT); //short, network order
inet_aton("10.12.110.57", &(my_addr.sin_addr));
memset(&(my_addr.sin_zero), '0', 8); // zero the padding

// inet_ntoa() - to print struct in_addr as numbers-and-dots notation
printf("%s", inet_ntoa(ina.sin_addr));




/**		SYSTEM CALLS OR BUST	*/
#include <sys/types.h>
#include <sys/socket.h>

int socket(int domain, int type, int protocol);
/**
*	socket(); - get the File Descriptor	
*
*	@domain		should be set to AF_xxx, AF_INET for example
*	@type		what kind of socket this is: SOCK_STREAM | SOCK_DGRAM
*	@protocol	set to 0, socket() will decide based on @type, or use getprotobyname();
*	
*	also should take a look into AF_INET and PF_INET, good to know, AF_INET everywhere should be fine too.
*/


#include <sys/types.h>
#include <sys/socket.h>

int bind(int sockfd, struct sockaddr *my_addr, int addrlen);
/**
*	bind() - What port am I on?
*
*	@sockfd		socket file descriptor returned by socket();
*	@my_addr	pointer to struct sockaddr which contains address, namely, port, IP.
*	@addrlen	can be set to sizeof(struct sockaddr)
*/

// Sample code
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 3900

main()
{
	int sockfd;
	struct sockaddr_in my_addr;
	
	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	my_addr.sin_family = AF_INET;		// host byte order
	my_addr.sin_port = htons(PORT);		// short, network byte order
	my_addr.sin_addr.s_addr = inet_addr("10.12.110.57");

	// automatically get your own IPaddress/port --- or maybe wrapping the value with htons(0) & htonl(INADDR_ANY)
	my_addr.sin_port = 0;					// choose an unused port at random
	my_addr.sin_addr.s_addr = INADDR_ANY;	// use my IP address

	memset(&(my_addr.size_zero), '\0', 8); // reset the rest of the struct;
	bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr));
	/** ... */
}

// ports are ranged from 1025 to 65535, below 1024 are RESERVED
/**
*	sometimes bind() resulted in "address already in use"
*	we can wait a bit, or actually reuse that port, like below
*/
int yes = 1;
if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
	perror("setsockopt");
	exit(1);
}


/** connect();     */
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define DEST_IP "10.12.110.57"
#define DEST_PORT 23

main()
{
	int sockfd;
	struct sockaddr_in dest_addr;		// will hold the destination addr

	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = DEST_PORT;
	dest_addr.sin_addr.s_addr = inet_addr(DEST_IP);
	memset(&(dest_addr.sin_zero), '\0', 8); // 0 the rest of the struct

	// int connect( int sockfd, struct sockaddr *serv_addr, int addrlen);
	connect(sockfd, &dest_addr, sizeof(struct sockaddr));
	// ...
}

/**
*	listen();
*	instead of connecting to them, we're just wait for comming connections.
*	
*	int listen( int sockfd, int backlog);
*/

/**
*	accept();
*	
*/
#include <sys/socket.h>
int accept( int sockfd, void *addr, int *addrlen);

// sample
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 3949
#define BACKLOG 10	// how many pending connections the queue will hold

main()
{
	int sockfd, new_fd;
	struct sockaddr_in my_addr;
	struct sockaddr_in their_addr;
	int sin_size;


	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(PORT);
	my_addr.sin_addr.s_addr = INADDR_ANY;
	memset(&(my_address.sin_zero), '\0', 8);

	bind(sockfd, (struct sockaddr *)&my_addr, sizeof(sockaddr_in));

	listen(sockfd, BACKLOG);

	sin_size = sizeof(struct sockaddr_in);
	new_fd = accept( sockfd, (struct sockaddr *)&their_addr, &sin_size);

	// ...
}


/**
*	send() & recv()
*	
*	these are TCP communication method 
*/

int send(int sockfd, const void *msg, size_t len, int flags);
int recv( int sockfd, void *buf, size_t len, int flags);

char *msg = "hello tcp";
int len, bytes_sent;

len = strlen(msg);
bytes_sent = send(sockfd, msg, len, 0);



/**
*	sendto() & recvfrom()
*
*	these are UDP methods
*	
*/

int sendto( int sockfd, const void *msg, size_t len, int flags, const struct sockaddr *to, int tolen);
int recvfrom( int sockfd, void *buf, size_t len, int flags, struct sockaddr *from, int *fromlen);

/**
*	close() & shutdown()
*	
*	close(sockfd);
*	
*	int shutdown( int sockfd, int how);
*	@how	0 further receives are disallowed
*			1 further sends are disallowed
*			2 further sends & receives are disallowed ( like close())
*/


/**
*	getpeername() & gethostname();
*	
*	
*/
#include <sys/socket.h>
int getpeername(int sockfd, struct sockaddr *addr, socklen_t *addrlen);

#include <unistd.h>
int gethostname(char *hostname, size_t size);

/* ------ DNS --------- 
*
*
*	DNS host lookup w telnet [hostname]
*	once telnet return the IP address, we can further use it w bind(), connect(), sendto() ...
*/

#include <netdb.h>
struct hostent *gethostbyname(const char *name);

struct hostent {
	char *h_name;
	char **h_aliases;
	int h_addrtype;
	int	h_length;
	char **h_addr_list;
}
#define h_addr h_addr_list[0];

/**
*	select(); 
*		- monitor multiple sockets at the same time.
*		- 
*	macros:
*		- FD_ZERO( fd_set *set)				- clear a fd set
*		- FD_SET( int fd, fd_set *set)		- adds fd to the set
*		- FD_CLR( int fd, fd_set *set)		- remove fd from set
*		- FD_ISSET( int fd, fd_set *set)	- test to see if fd is in the set
*/

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

int select( int numfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);

struct timeval {
	int tv_sec;
	int tv_usec;
};



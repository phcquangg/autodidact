#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <linux/if.h>
#include <linux/if_tun.h>

#define BUFFER_SIZE 2048

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

	if (*dev_name) strncpy(ifr.ifr_name, dev_name, IFNAMSIZ - 1);

	if ((err = ioctl(fd, TUNSETIFF, (void *)&ifr)) < 0)
	{
		perror("ioctl(TUNSETIFF) failed");
		close(fd);
		return err;
	}

	return fd;
}

int main()
{
	char tap0_name[] = "tap0";
	char tap1_name[] = "tap1";

	int tap0_fd = tap_alloc(tap0_name);
	int tap1_fd = tap_alloc(tap1_name);

	if (tap0_fd < 0 || tap1_fd < 0)
	{
		fprintf(stderr, "Failed to initialize TAP interfaces.\n");
		return 1;
	}

	printf("[+] Successfully bound to %s (fd=%d) and %s (fd=%d)\n", tap0_name, tap0_fd, tap1_name, tap1_fd);
	printf("[+] Listening for raw Ethernet frames...");

	unsigned char buffer[BUFFER_SIZE];
	int max_fd = (tap0_fd > tap1_fd) ? tap0_fd : tap1_fd;

	while (1)
	{
		fd_set rd_set;
		FD_ZERO(&rd_set);
		FD_SET(tap0_fd, &rd_set);
		FD_SET(tap1_fd, &rd_set);

		int ret = select(max_fd + 1, &rd_set, NULL, NULL, NULL);
		
		if (ret < 0)
		{
			perror("select() error");
			break;
		}

		if (FD_ISSET(tap0_fd, &rd_set))
		{
			ssize_t nread = read(tap0_fd, buffer, sizeof(buffer));

			if (nread > 0)
			{
				printf("[tap0 -> Router] Captured %zd bytes\n", nread);
				printf(" src MAC: %02x:%02x:%02x:%02x:%02x:%02x -> dest MAC: %02x:%02x:%02x:%02x:%02x:%02x | EtherType: 0x%02x%02x\n",
						buffer[6], buffer[7], buffer[8], buffer[9], buffer[10], buffer[11],
						buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5],
						buffer[12], buffer[13]);
			}
		}

		if (FD_ISSET(tap1_fd, &rd_set))
		{
			ssize_t nread = read(tap1_fd, buffer, sizeof(buffer));
				
			if (nread > 0)
			{
				printf("[tap1 -> Router] Captured %zd bytes\n", nread);
				printf("  Src MAC: %02x:%02x:%02x:%02x:%02x:%02x -> Dest MAC: %02x:%02x:%02x:%02x:%02x:%02x | EtherType: 0x%02x%02x\n",
                    buffer[6], buffer[7], buffer[8], buffer[9], buffer[10], buffer[11],
                    buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5],
                    buffer[12], buffer[13]);
			}
		}
	
	close(tap0_fd);
	close(tap1_fd);

	return 0;
}

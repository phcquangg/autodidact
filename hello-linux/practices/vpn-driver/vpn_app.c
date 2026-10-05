#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define DEV_PATH "dev/tun_custom"
#define BUFFER_SIZE 2048

int main()
{
	int fd = open(DEV_PATH, O_RDWR);
	if (fd < 0) {
		perror("Failed to open device file");
		return 1;
	}

	char buffer[BUFFER_SIZE];

	ssize_t bytes_read = read( fd, buffer, sizeof(buffer));
	if (bytes_read > 0) {
		printf("Received %zd bytes from kernel space.\n", bytes_read);

		for (int i = 0; i < bytes_read; i++) {
			buffer[i] ^= 0x5A;
		}

		for (int i = 0; i < bytes_read; ++i) {
			buffer[i] ^= 0x5A;
		}

		ssize_t bytes_written = write(fd, buffer, bytes_read);
		printf("Reinjected %zd bytes back to kernel space\n", bytes_written);
	}

	close(fd);
	return 0;
}



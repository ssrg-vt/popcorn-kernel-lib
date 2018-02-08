#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <string.h>
#include <fcntl.h>

#include "migrate.h"

void make_busy(void)
{
	int i;
	unsigned long sum = 0;
	for (i = 0; i < 10000; i++) {
		sum += i;
	}
}

void pingpong(void)
{
	int value = 0;
	char buffer[80];

	while (1) {
		int fd;
		
		make_busy();

		fd = open("/tmp/ping", O_RDONLY);
		if (fd < 0) {
			printf("waiting for the control file\n");
			continue;
		}
		read(fd, buffer, sizeof(buffer));
		close(fd);

		if (atoi(buffer) != value) {
			continue;
		}
		value = !value;

		printf("Jump to %d!\n", value);
		migrate(value, NULL, NULL);
	}
}

int main(int argc, char *argv[])
{
	printf("ping pongx\n");
	pingpong();

	return 0;
}

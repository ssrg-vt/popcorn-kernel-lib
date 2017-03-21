#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <string.h>
#include <fcntl.h>

#include "popcorn.h"

void loop(void)
{
	int value = 0;
	while (1) {
		char buffer[80];
		int fd = 0;
		int jump = 0;
		int i;
		int xx = 0;
		for (i = 0; i < 1000000; i++) {
			xx += i;
		}
		fd = open("/home/beowulf/share/ping", O_RDONLY);
		if (fd < 0) {
			printf("waiting for the file\n");
			continue;
		}
		read(fd, buffer, sizeof(buffer));
		if (atoi(buffer) == value) {
			value = !value;
			jump = 1;
		}
		close(fd);

		if (jump) {
			//printf("Jump!\n");
			popcorn_migrate_this(value);
		}
	}
}

int main(int argc, char *argv[])
{
	printf("Loop at %lx\n", (unsigned long)loop);
	if (argc == 1) {
		loop();
	}

	printf("Hello world\n");
	printf("What the!\n");

	return 0xdeadbeaf;
}

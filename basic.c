#define _GNU_SOURCE

#include <stdio.h>
#include <sched.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <string.h>
#include <fcntl.h>

#include "popcorn.h"

void loop(void)
{
	int a = 0;
	int b = 1;
	int c = a + b;
	int z;
	int fd;

	fd = open("/home/beowulf/test/x", O_CREAT | O_WRONLY | O_TRUNC, 0666);
	write(fd, "HELLO!", 7);
	close(fd);
	return;
}

int main(int argc, char *argv[])
{
	unsigned long rbp, rsp;

	if (argc == 1) {
		printf("Migrate\n", argc);
		popcorn_migrate_this(0);
		loop();
		popcorn_migrate_this(0);
	}

	printf("Hello world\n");
	printf("What the!\n");

	return 0xdeadbeaf;
}

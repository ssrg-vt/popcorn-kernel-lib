#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>

#include "popcorn.h"

void migrated_callback(void *params)
{
	char *buffer = params;
	int fd;
	fd = open("/tmp/basic-output", O_CREAT | O_WRONLY | O_APPEND, 0666);
	write(fd, buffer, strlen(buffer));
	close(fd);

	return;
}

int main(int argc, char *argv[])
{
	time_t t;
	struct tm *now;
	char param[80];

	t = time(NULL);
	now = localtime(&t);
	strftime(param, sizeof(param), "%Y%m%d_%H%M%S\n", now);

	printf("Migrate me out!\n");

	migrate(1, migrated_callback, param);
	sleep(1);
	migrate(0, migrated_callback, param);

	printf("I am back!\n");

	return 0xdead;
}

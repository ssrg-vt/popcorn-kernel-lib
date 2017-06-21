#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>

#include <sched.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <string.h>
#include <fcntl.h>
#include <string.h>

#include "popcorn.h"

void loop(void)
{
	sleep(3);
}

void migrated_callback(void *params)
{
	char *buffer = params;
	int fd;
	fd = open("/home/beowulf/basic-output", O_CREAT | O_WRONLY, 0666);
	write(fd, buffer, strlen(buffer));
	close(fd);

	return;
}

int main(int argc, char *argv[])
{
	/*
	int proposed;
	pid_t pid = getpid();

	popcorn_propose_migration(pid, 1);
	proposed = popcorn_migration_proposed();
	printf("Migration is %sproposed\n", proposed ? "" : "not ");
	*/

	if (argc == 1) {
		char *param = (char *)malloc(4096);

		sprintf(param, "WHAT THE HECK is this?");

		printf("Migrate me!\n");
		migrate(1, migrated_callback, param);
		loop();

		migrate(0, migrated_callback, param);
		free(param);
	}

	printf("Welcome back to the origin\n");

	return 0xdeadbeaf;
}

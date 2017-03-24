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

	fd = open("/home/beowulf/basic-output", O_CREAT | O_WRONLY | O_TRUNC, 0666);
	write(fd, "HELLO!", 7);
	close(fd);
	return;
}

int main(int argc, char *argv[])
{
	int migrate;
	pid_t pid = getpid();

	popcorn_propose_migration(pid, 1);

	migrate = popcorn_migration_proposed();

	printf("Migration is %sproposed\n", migrate ? "" : "not ");

	if (argc == 1) {
		printf("Migrate me!\n");
		popcorn_migrate_this(0);
		loop();
		popcorn_migrate_this(0);
	}

	printf("Welcome back to the origin\n");

	return 0xdeadbeaf;
}

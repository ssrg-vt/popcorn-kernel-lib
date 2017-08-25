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

#ifdef __x86_64__
#define GET_RBP(rbp) asm volatile ("mov %%rbp, %0" : "=g"(rbp))
#elif __aarch64__
#define GET_RBP(rbp) asm volatile ("str x29, %0" : "=m"(rbp))
#elif __powerpc64__
#define GET_RBP(rbp) asm volatile ("mflr 22; std 22, %0" : "=m"(rbp): : "r22")
#else
#error Does not support this architecture
#endif

void migrated_callback(void *params)
{
	char *buffer = params;
	int fd;
	fd = open("/home/beowulf/basic-output",
			O_CREAT | O_WRONLY | O_APPEND, 0666);
	write(fd, buffer, strlen(buffer));
	close(fd);

	return;
}

int return_from(int n, int z)
{
	unsigned long *rbp = NULL;
	int i;
	int x = 10;
	int y[16] = {0};

	for (i = 0; i < 16; i++) {
		y[i] = i;
		z += y[i];
	}
	if (--n == 0) {
		GET_RBP(rbp);
		printf("%p %lx %p\n", rbp, *rbp, __builtin_frame_address(0));
		rbp = (unsigned long *)*rbp;
		printf("%p %lx %p\n", rbp, *rbp, __builtin_frame_address(1));
		rbp = (unsigned long *)*rbp;
		printf("%p %lx %p\n", rbp, *rbp, __builtin_frame_address(2));
		migrate(0, NULL, NULL);

		GET_RBP(rbp);
		printf("%p %lx %p\n", rbp, *rbp, __builtin_frame_address(0));
	} else {
		x += return_from(n, 0) + z;
	}
	return x;
}

int main(int argc, char *argv[])
{
	time_t t;
	struct tm *now;
	char param[80];
	unsigned long *rbp;

	t = time(NULL);
	now = localtime(&t);
	strftime(param, sizeof(param), "%Y%m%d_%H%M%S\n", now);

	printf("Migrate me out!\n");

	GET_RBP(rbp);
	printf("%p %lx\n", rbp, *rbp);

	migrate(1, migrated_callback, param);
	//sleep(1);

	return_from(3, 0);
	//migrate(0, migrated_callback, param);

	printf("I am back!\n");
	//sleep(2);
	//goto again;

	return 0xdeaf;
}

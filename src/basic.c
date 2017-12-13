#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

#include "popcorn.h"

int main(int argc, char *argv[])
{
	unsigned long delay_ms;

	if (argc == 2) {
		delay_ms = atol(argv[1]);
	} else {
		struct timeval time;

		gettimeofday(&time, NULL);
		srandom(time.tv_sec * time.tv_usec);
		delay_ms = random() % 200 + 1; /* Up to 200 ms */
	}

	printf("Travel for %ld ms\n", delay_ms);
	migrate(1, NULL, NULL);
	usleep(delay_ms * 1000);
	migrate(0, NULL, NULL);

	return 0;
}

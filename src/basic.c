#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

#include "migrate.h"

int main(int argc, char *argv[])
{
	unsigned long delay_ms;
	int ret;

	if (argc == 2) {
		delay_ms = atol(argv[1]);
	} else {
		struct timeval time;

		gettimeofday(&time, NULL);
		srandom(time.tv_sec * time.tv_usec);
		delay_ms = random() % 200 + 1; /* Up to 200 ms */
	}

	printf("Travel for %ld ms\n", delay_ms);
	ret = migrate(1, NULL, NULL);
	if (ret) goto out_ret;

	usleep(delay_ms * 1000);
	ret = migrate(0, NULL, NULL);

	return ret;
out_ret:
	fprintf(stderr, "Cannot migrate to 1: %d\n", ret);
	return ret;
}

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>

#include "popcorn.h"

static int THREADS = 2;
static int BUFFER = 1; /* in MB */
static char *buffer = NULL;
static int LOOPS = 10;
static int coresPerNode = 4;

pthread_barrier_t barrier;

struct child_param {
	pthread_t thread_info;
	int tid;
	int ret;
	int stop;
};

unsigned long long loop_r()
{
	int i;
	unsigned long long sum = 0;
	for (i = 0; i < BUFFER * 1024 * 1024; i++) {
		sum += buffer[i];
	}
	return sum;
}
int loop_w(void)
{
	int i;
	for (i = 0; i < BUFFER * 1024 * 1024; i++) {
		buffer[i]++;
	}
	return 0;
}
void loop_rw(void)
{
	//const int pages = BUFFER * 1024 / 4;
	int i;
	for (i = 0; i < 1000; i++) {
	}
}


void *child(void *arg)
{
	struct child_param *param = (struct child_param *)arg;
	int tid = param->tid;
	int i;

	if (tid / coresPerNode) migrate(tid / coresPerNode, NULL, NULL);
	for (i = 0; i < LOOPS; i++) {
		pthread_barrier_wait(&barrier);
		loop_r();
		pthread_barrier_wait(&barrier);
	}
	if (tid / coresPerNode) migrate(0, NULL, NULL);

	return 0;
}

void init_buffer(void)
{
	int i;
	printf("Initialize buffer...");
	fflush(stdout);
	posix_memalign((void **)&buffer, 4096, BUFFER * 1024 * 1024);

	for (i = 0; i < BUFFER * 1024 * 1024; i++) {
		buffer[i] = rand();
	}
	printf(" Done\n");
}

int main(int argc, char *argv[])
{
	struct child_param *threads;
	int i;

	if (argc >= 2) {
		THREADS = atoi(argv[1]);
	}
	if (argc >= 3) {
		BUFFER = atoi(argv[2]);
	}
	printf("Threads: %d\n", THREADS);
	printf("Buffer:  %d MB\n", BUFFER);

	threads = malloc(sizeof(*threads) * THREADS);
	init_buffer();

	pthread_barrier_init(&barrier, NULL, THREADS + 1);
	for (i = 0; i < THREADS; i++) {
		threads[i].tid = i;
		threads[i].ret = 0;
		threads[i].stop = 0;
		pthread_create(
				&threads[i].thread_info, NULL, &child, threads + i);
	}

	for (i = 0; i < LOOPS; i++) {
		struct timeval start, end;
		unsigned long time;

		gettimeofday(&start, NULL);
		pthread_barrier_wait(&barrier);
		pthread_barrier_wait(&barrier);
		gettimeofday(&end, NULL);

		time = end.tv_sec * 1000000 + end.tv_usec
			- (start.tv_sec * 1000000 + start.tv_usec);
		printf("%lu.%lu\n", time / 1000000, time % 1000000);
	}

	for (i = 0; i < THREADS; i++) {
		pthread_join(threads[i].thread_info, (void **)&(threads[i].ret));
		printf("Exited %d %d with %d\n", i, threads[i].tid, threads[i].ret);
	}

	free(buffer);
	free(threads);
	return 0;
}

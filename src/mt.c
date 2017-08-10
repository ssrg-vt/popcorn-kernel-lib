#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <fcntl.h>

#include "popcorn.h"

const int THREADS = 32;
const int LOOPS = 100;

pthread_barrier_t barrier_start;
pthread_barrier_t barrier_end;
pthread_barrier_t barrier_loop;

struct child_param {
	pthread_t thread_info;
	int tid;
	int ret;
	int stop;
};

int loop()
{
	sleep(1);
	return 0;
}

void *child(void *arg)
{
	struct child_param *param = (struct child_param *)arg;
	int tid = syscall(SYS_gettid);
	int i;

	for (i = 0; i < LOOPS; i++) {
		printf("Entering %d %d\n", param->tid, tid);
		pthread_barrier_wait(&barrier_start);
		printf("Entered %d %d\n", param->tid, tid);
		migrate(1, NULL, NULL);
		loop();
		migrate(0, NULL, NULL);
		pthread_barrier_wait(&barrier_end);
		printf("Exited %d %d\n", param->tid, tid);
		pthread_barrier_wait(&barrier_loop);
	}

	return 0;
}

int main(int argc, char *argv[])
{
	struct child_param threads[THREADS];
	int i;

	pthread_barrier_init(&barrier_start, NULL, THREADS + 1);
	pthread_barrier_init(&barrier_end, NULL, THREADS + 1);

	for (i = 0; i < THREADS; i++) {
		threads[i].tid = i;
		threads[i].ret = 0;
		threads[i].stop = 0;
		pthread_create(
				&threads[i].thread_info, NULL, &child, threads + i);
	}

	for (i = 0; i < LOOPS; i++) {
		pthread_barrier_wait(&barrier_start);
		pthread_barrier_init(&barrier_loop, NULL, THREADS + 1);
		pthread_barrier_wait(&barrier_end);

		pthread_barrier_init(&barrier_start, NULL, THREADS + 1);
		pthread_barrier_wait(&barrier_loop);
		pthread_barrier_init(&barrier_end, NULL, THREADS + 1);
	}

	for (i = 0; i < THREADS; i++) {
		pthread_join(threads[i].thread_info, (void **)&(threads[i].ret));
		printf("Exited %d %d with %d\n", i, threads[i].tid, threads[i].ret);
	}
	return 0;
}

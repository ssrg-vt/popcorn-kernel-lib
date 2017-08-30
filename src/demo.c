#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>

#include "popcorn.h"

int nthreads = 16;
int nodes = 3;
int here = 0;
int check_stall = 1;
unsigned long *global;
pthread_barrier_t barrier_start;

struct thread {
	pthread_t thread_info;
	int id;
	int tid;
	int at;
	int ret;
	int finish;
	unsigned long count;
};
struct thread **threads;
struct thread stall_monitor;

int make_busy(int tid, struct thread *t)
{
	/*
	int i;
	int xx = 1;
	for (i = 0; i < 10000; i++) {
		xx *= 7;
	}
	*/
	t->count++;
	global[tid]++;
	return t->count;
}

static int __get_location(int tid)
{
	return threads[tid]->at;
}

static void *child_thread(void *_thread)
{
	struct thread *thread = _thread;
	int tid = thread->id;
	int prev_nid = __get_location(thread->id);
	thread->tid = syscall(SYS_gettid);

	pthread_barrier_wait(&barrier_start);

	while (!thread->finish) {
		int new_nid = __get_location(thread->id);

		if (prev_nid != new_nid) {
			//printf("Move %d %d --> %d", thread->id, prev_nid, new_nid);
			migrate(new_nid, NULL, NULL);

			if (new_nid == here) {
				printf("Welcome back thread %d\n", tid);
			}
		}
		prev_nid = new_nid;
		make_busy(tid, thread);
	}

	return 0;
}

static void __print_thread_status(void)
{
	int i;

	printf("----+-");
	for (i = 0; i < nthreads; i++) {
		printf("----");
	}
	printf("\n");

	printf("TID | ");
	for (i = 0; i < nthreads; i++) {
		printf("%3d ", i);
	}
	printf("\n");

	printf("----+-");
	for (i = 0; i < nthreads; i++) {
		printf("----");
	}
	printf("\n");

	printf(" AT | ");
	for (i = 0; i < nthreads; i++) {
		printf("%3d ", threads[i]->at);
	}
	printf("\n");

	printf("----+-");
	for (i = 0; i < nthreads; i++) {
		printf("----");
	}
	printf("\n");
}


static void __print_ps(void)
{
	char buffer[4096];
	int fd = open("/proc/popcorn_ps", O_RDONLY);
	read(fd, buffer, sizeof(buffer));
	close(fd);

	printf("%s\n", buffer);
	return;
}


static void __print_heartbeats(void)
{
	int i;
	for (i = 0; i < nthreads; i++) {
		printf("%3ld ", (threads[i]->count / 1000) % 1000);
	}
	printf("\n");
}

static int __migrate_thread(int tid, int nid)
{
	threads[tid]->at = nid;

	return 0;
}


static int __init_thread_params(void)
{
	const int PAGE_SIZE = 4096;
	int i;
	threads = (struct thread **)malloc(sizeof(struct thread *) * nthreads);
	global = (unsigned long *)malloc(sizeof(unsigned long) * nthreads);

	for (i = 0; i < nthreads; i++) {
		global[i] = 0;
		posix_memalign((void **)&(threads[i]), PAGE_SIZE, sizeof(struct thread));
	}

	pthread_barrier_init(&barrier_start, NULL, nthreads + 1);
	return 0;
}

static void *stall_monitor_thread(void *arg)
{
	unsigned long *last =
		(unsigned long *)malloc(sizeof(unsigned long) * nthreads);

	while (!stall_monitor.finish) {
		int i;
		sleep(2);
		for (i = 0; i < nthreads; i++) {
			if (last[i] == threads[i]->count) {
				fprintf(stderr, "Thread %d might be stalled %d\n", i,
						threads[i]->tid);
			}
			last[i] = threads[i]->count;
		}
	}
	free(last);

	return NULL;
}


int main(int argc, char *argv[])
{
	int i;
	int finish = 0;
	struct thread *thread;
	int opt;
	int skip_print = 0;

	while ((opt = getopt(argc, argv, "t:n:h:D")) != -1) {
		switch(opt) {
		case 't':
			nthreads = atoi(optarg);
			break;
		case 'n':
			nodes = atoi(optarg);
			break;
		case 'h':
			here = atoi(optarg);
			break;
		case 'D':
			check_stall = 0;
			break;
		}
	}

	printf("======================================================\n");
	printf("     Welcome to the thread migration console.\n");
	printf("======================================================\n");
	printf("- # of threads: %d\n", nthreads);
	printf("- # of nodes  : %d\n", nodes);
	printf("- Here        : %d\n", here);
	printf("\n");

	__init_thread_params();

	for (i = 0; i < nthreads; i++) {
		thread = threads[i];

		thread->id = i;
		thread->tid = 0;
		thread->at = here;
		thread->ret = 0;
		thread->count = 0;
		thread->finish = 0;

		pthread_create(&thread->thread_info, NULL, &child_thread, thread);
	}
	if (check_stall) {
		stall_monitor.finish = 0;
		pthread_create(&stall_monitor.thread_info, NULL, &stall_monitor_thread, &stall_monitor);
	}

	pthread_barrier_wait(&barrier_start);
	for (i = 0; i < nthreads; i++) {
		printf("Thread %2d [%d] created at %p\n", i, threads[i]->tid, threads[i]);
	}


	/* Main loop */
	while (!finish) {
		int tid;
		int to;
		int from;
		int ret;

		if (!skip_print) {
			__print_thread_status();
		}
		skip_print = 0;

		printf("Thread ID : ");
		scanf("%d", &tid);

		if (tid == -1 || tid == 999) {
			break;
		}
		if (tid == 99) {
			continue;
		}
		if (tid == 88) {
			__print_ps();
			skip_print = 1;
			continue;
		}
		if (tid == 77) {
			__print_heartbeats();
			skip_print = 1;
			continue;
		}
		if (tid < 0 || tid >= nthreads) {
			fprintf(stderr, "ERROR: Thread ID must be between 0 to %d\n",
					nthreads - 1);
			continue;
		}
		from = threads[tid]->at;

		if (from != here) {
			printf("Bring back %d from %d\n", tid, from);
			to = here;
		} else {
			printf("       To : ");
			scanf("%d", &to);
			printf("\n");

			if (to < 0 || to >= nodes) {
				fprintf(stderr,
						"ERROR: Destination must be between 0 to %d\n",
						nodes - 1);
				continue;
			}

			if (to == from) {
				fprintf(stderr,
						"ERROR: Destination should be different from "
						"the current location (%d)\n", from);
				continue;
			}
		}

		if ((ret = __migrate_thread(tid, to))) {
		}
	}

	/* Clean up */
	if (check_stall) {
		stall_monitor.finish = 1;
		pthread_join(stall_monitor.thread_info, (void **)&(stall_monitor.ret));
	}

	for (i = 0; i < nthreads; i++) {
		if (threads[i]->at != here) {
			__migrate_thread(i, here);
		}
		threads[i]->finish = 1;
	}
	for (i = 0; i < nthreads; i++) {
		thread = threads[i];
		pthread_join(thread->thread_info, (void **)&(thread->ret));
		printf("Exited %d with %d\n", thread->id, thread->ret);
		free(thread);
	}
	free(threads);
	free(global);

	return 0;
}

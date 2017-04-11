#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>

#include "popcorn.h"

const char * const PATH = "/home/beowulf/share/demo";
const int PAGE_SIZE = 4096;

int nthreads = 16;
int nodes = 4;
int here = 0;

struct thread {
	pthread_t thread_info;
	int id;
	int at;
	int ret;
	int finish;
};
struct thread **threads;

int make_busy(void)
{
	int i;
	int xx = 1;
	for (i = 0; i < 10000; i++) {
		xx *= 7;
	}
	return xx;
}

static int __get_location(int tid)
{
	return threads[tid]->at;
}

static int __put_location(int tid, int prev_nid, int nid)
{
#if 0
	char path[80];
	char buffer[80];
	int fd;

	sprintf(path, "%s/%d-", PATH, tid);
	sprintf(buffer, "%d --> %d", prev_nid, nid);

	fd = open(path, O_CREAT | O_WRONLY | O_TRUNC, 0666);
	write(fd, buffer, strlen(buffer));
	close(fd);
	sync();
#endif

	return 0;
}

static void *child(void *_thread)
{
	struct thread *thread = _thread;
	int tid = thread->id;
	int prev_nid = __get_location(thread->id);

	//printf("Thread %d started from %d\n", thread->id, thread->at);

	while (!thread->finish) {
		int new_nid = __get_location(thread->id);
		__put_location(tid, prev_nid, new_nid);

		if (prev_nid != new_nid) {
			//printf("Move %d %d --> %d", thread->id, prev_nid, new_nid);
			popcorn_migrate_this(new_nid);

			if (new_nid == here) {
				printf("Welcome back thread %d\n", tid);
			}
		}
		prev_nid = new_nid;
		make_busy();
	}

	return 0;
}

static void __print_thread_status(void)
{
	int i;

	printf("----+-");
	for (i = 0; i < nthreads; i++) {
		printf("----", threads[i]->at);
	}
	printf("\n");

	printf("TID | ");
	for (i = 0; i < nthreads; i++) {
		printf("%3d ", i);
	}
	printf("\n");

	printf("----+-");
	for (i = 0; i < nthreads; i++) {
		printf("----", threads[i]->at);
	}
	printf("\n");

	printf(" AT | ");
	for (i = 0; i < nthreads; i++) {
		printf("%3d ", threads[i]->at);
	}
	printf("\n");

	printf("----+-");
	for (i = 0; i < nthreads; i++) {
		printf("----", threads[i]->at);
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


static int __write_to_control_file(int tid, int nid)
{
	return 0;
	char path[80];
	char value[10];
	int fd;

	sprintf(path, "%s/%d", PATH, tid);
	sprintf(value, "%d", nid);

	fd = open(path, O_CREAT | O_WRONLY | O_TRUNC, 0666);
	if (fd < 0) {
		fprintf(stderr, "ERROR: Cannot open control file at %s (%d)\n",
				path, errno);
		return -errno;
	}
	write(fd, value, strlen(value));

	close(fd);
	sync();
}

static int __migrate_thread(int tid, int nid)
{
	threads[tid]->at = nid;

	return 0;
}


static int __init_thread_params(void)
{
	int i;
	threads = (struct thread **)malloc(sizeof(struct thread *) * nthreads);
	printf("%p\n", threads);

	for (i = 0; i < nthreads; i++) {
		posix_memalign((void **)&(threads[i]), PAGE_SIZE, sizeof(struct thread));
		printf("%d %p\n", i, threads[i]);
	}
}

int main(int argc, char *argv[])
{
	int i;
	int finish = 0;
	struct thread *thread;
	int opt;
	int skip_print = 0;

	while ((opt = getopt(argc, argv, "t:n:h:")) != -1) {
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
		int ret;
		thread = threads[i];

		thread->id = i;
		thread->at = here;
		thread->ret = 0;
		thread->finish = 0;

		__write_to_control_file(i, here);

		ret = pthread_create(&thread->thread_info, NULL, &child, thread);
	}

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
	}

	return 0;
}

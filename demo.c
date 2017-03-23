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

void busy(void)
{
	int i;
	int xx = 0;
	for (i = 0; i < 1000000; i++) {
		xx += i;
	}
}

static int __get_location(int tid)
{
	char path[80];
	char buffer[80];
	int at;
	int fd;

	sprintf(path, "%s/%d", PATH, tid);
	do {
		fd = open(path, O_RDONLY);
		if (fd < 0) {
			printf("waiting for the file %s\n", path);
			sleep(1);
			continue;
		}
	} while (fd < 0);
	read(fd, buffer, sizeof(buffer));
	at = atoi(buffer);
	close(fd);
	return at;
}

static void *child(void *_thread)
{
	struct thread *thread = _thread;
	int prev_nid = __get_location(thread->id);

	//printf("Thread %d started from %d\n", thread->id, thread->at);

	while (!thread->finish) {
		int new_nid = __get_location(thread->id);
		if (new_nid != prev_nid) {
			prev_nid = new_nid;
			popcorn_migrate_this(new_nid);
		}
		sleep(1);
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

static int __write_to_control_file(int tid, int nid)
{
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
}

static int __migrate_thread(int tid, int nid)
{
	__write_to_control_file(tid, nid);

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
		//printf("%p\n", threads[i]);
	}
}

int main(int argc, char *argv[])
{
	int i;
	int finish = 0;
	struct thread *thread;
	int opt;

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

		__print_thread_status();

		printf("Thread ID : ");
		scanf("%d", &tid);
		
		if (tid == -1 || tid == 99) {
			break;
		}
		if (tid == 999) {
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
		// printf("Exited %d with %d\n", thread->id, thread->ret);
	}

	return 0;
}

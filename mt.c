#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <fcntl.h>

#include "popcorn.h"

const int THREADS = 4;

struct child_param {
	pthread_t thread_info;
	int tid;
	int local;
	int ret;
};

int loop(int tid)
{
	int i = 0;
	char path[80] = {0};
	char buffer[80] = {0};
	int fd;

again:
	snprintf(buffer, sizeof(buffer), "%d was here %d\n", tid, i++);
	snprintf(path, sizeof(path), "/home/beowulf/test/%d", tid);

	fd = open(path, O_CREAT | O_WRONLY | O_TRUNC, 0666);
	write(fd, buffer, strlen(buffer));
	close(fd);

	sleep(1);
	goto again;

	return 0;
}

void *child(void *arg)
{
	struct child_param *param = (struct child_param *)arg;
	pid_t pid = getpid();
	pid_t tid = syscall(SYS_gettid);

	printf("Entering %d %d\n", pid, tid);
	if (!param->local && param->tid < (THREADS / 2))
		popcorn_migrate_this(1);
	loop(tid);

	if (!param->local && param->tid < (THREADS / 2))
		popcorn_migrate_this(0);
	printf("Exiting %d %d\n", pid, tid);

	return (void *)(unsigned long)tid;
}

int main(int argc, char *argv[])
{
	struct child_param threads[THREADS];
	int i;

	for (i = 0; i < THREADS; i++) {
		threads[i].tid = i;
		threads[i].ret = 0;
		threads[i].local = (argc != 1);
		int result = pthread_create(
				&threads[i].thread_info, NULL, &child, threads + i);
		//sleep(1);
	}

	for (i = 0; i < THREADS; i++) {
		pthread_join(threads[i].thread_info, (void **)&(threads[i].ret));
		printf("Exited: %d %d\n", threads[i].tid, threads[i].ret);
	}
	return 0;
}

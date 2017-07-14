#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <fcntl.h>

#include "popcorn.h"

const int THREADS = 6;
const int NODES = 3;
int local = 0;

struct child_param {
	pthread_t thread_info;
	int tid;
	int remote;
	int ret;
};

int loop(int tid)
{
	char path[80];
	char buffer[80];
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
	pid_t tid = syscall(SYS_gettid);

	printf("Entering %d to %d\n", tid, param->remote);
	migrate(1, NULL, NULL);
	loop(tid);
	migrate(0, NULL, NULL);
	printf("Exiting %d\n from %d", tid, param->remote);

	return (void *)(unsigned long)tid;
}

int main(int argc, char *argv[])
{
	struct child_param threads[THREADS];
	int i;
	int remote = 0;
	local = (argc != 1);

	for (i = 0; i < THREADS; i++) {
		threads[i].tid = i;
		threads[i].ret = 0;
		threads[i].remote = (remote++) % NODES;
		pthread_create(
				&threads[i].thread_info, NULL, &child, threads + i);
		//sleep(1);
	}

	for (i = 0; i < THREADS; i++) {
		pthread_join(threads[i].thread_info, (void **)&(threads[i].ret));
		printf("Exited %d with %d\n", threads[i].tid, threads[i].ret);
	}
	return 0;
}

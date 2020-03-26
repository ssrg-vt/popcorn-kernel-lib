#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>

#include "migrate.h"

int nthreads = 1;
int nodes = 2;
int here = 0;
int check_stall = 1;
unsigned long *global;
pthread_barrier_t barrier_start;
static const char *arch_sz[] = {
        "unknown",
        "arm64",
        "x86-64",
        "ppc64le",
        NULL,
};

struct thread {
	pthread_t thread_info;
	int id;
	pid_t tid;
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

static int __migrate_thread(int tid, int nid)
{
    threads[tid]->at = nid;

    return 0;
}

static void *child_thread(void *_thread)
{
	struct thread *thread = _thread;
	int tid = thread->id;
	int prev_nid = __get_location(thread->id);
	thread->tid = popcorn_gettid();
    struct popcorn_thread_status status;

	pthread_barrier_wait(&barrier_start);

	while (!thread->finish) {
		int new_nid = __get_location(thread->id);

		if (prev_nid != new_nid) {

			printf("Move %d %d --> %d\n", thread->tid, prev_nid, new_nid);
            printf("\n** Current thread status **\n");
            if (popcorn_get_status(&status)) {
                perror(" Cannot retrieve the current thread's status");
                break;
            }
            printf("Thread %d - currently at node %d\n", status.peer_pid, status.current_nid);
//            printf("migration destination is ");
//            if (status.proposed_nid == -1) {
//                printf("not proposed\n");
//            } else {
//                printf("proposed to %d\n", status.proposed_nid);
//            }
            printf("peer node is %d\n\n", status.peer_nid);

            migrate(new_nid, NULL, NULL);

			if (new_nid == here) {
				printf("Welcome back thread %d\n", tid);
			}
		}
		prev_nid = new_nid;
		make_busy(tid, thread);
	}

	return NULL;
}

static void __print_thread_status(void)
{
	int i;

	printf("----+-");
	for (i = 0; i < nthreads; i++) {
		printf("----");
	}
	printf("\n");

	printf(" TID | ");
	for (i = 0; i < nthreads; i++) {
		printf("%3d ", threads[i]->tid);
	}
	printf("\n");

	printf("----+-");
	for (i = 0; i < nthreads; i++) {
		printf("----");
	}
	printf("\n");

    printf(" ITR | ");
    for (i = 0; i < nthreads; i++) {
        printf("%3d ", threads[i]->id);
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

    printf(" LOC | ");
    for (i = 0; i < nthreads; i++) {
        printf("%3p ", threads[i]);
    }
    printf("\n");

    printf("----+-");
    for (i = 0; i < nthreads; i++) {
        printf("----");
    }
    printf("\n");

}


static int __print_ps(void)
{
	char buffer[4096];
	int demo_errno = 0;
	int fd = open("/proc/popcorn_ps", O_RDONLY);
	if(read(fd, buffer, sizeof(buffer)) > 0)
    {
	    demo_errno = -3;
    };
	close(fd);

	printf("%s\n", buffer);
	return demo_errno;
}


static void __print_heartbeats(void)
{
	int i;
	for (i = 0; i < nthreads; i++) {
		printf("%3ld ", (threads[i]->count / 1000) % 1000);
	}
	printf("\n");
}


static int __init_thread_params(void)
{
	int i;
	int demo_errno;
	threads = (struct thread **)malloc(sizeof(struct thread *) * nthreads);
	global = (unsigned long *)malloc(sizeof(unsigned long) * nthreads);

	for (i = 0; i < nthreads; i++) {
		global[i] = 0;
		if((demo_errno = posix_memalign((void **)&(threads[i]), PAGE_SIZE, sizeof(struct thread))))
        {
		    return demo_errno;
        };
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
				fprintf(stderr, "Thread %d might be stalled %d last count %lu\n", i,
						threads[i]->tid, last[i]);
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
    struct popcorn_node_info pnodes[MAX_POPCORN_NODES];
	int opt;
	int skip_print = 0;
	int demo_errno = 0;
    int current_nid;
    int ret;

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
			check_stall = atoi(optarg);
			break;
		}
	}

	printf("======================================================\n");
	printf("     Welcome to the thread migration console.\n");
	printf("======================================================\n");

    if ((demo_errno = popcorn_get_node_info(&current_nid, pnodes, MAX_POPCORN_NODES))) {
        perror(" FAILED: popcorn_get_node_info, Cannot retrieve the nodes' information\n");
        return demo_errno;
    }
    if(here != current_nid){
        here = current_nid;
    }


	printf("- # of threads: %d\n", nthreads);
	printf("- # of nodes  : %d\n", nodes);
	printf("- Here        : %d\n", here);
	printf("\n");

	if((demo_errno = __init_thread_params()))
    {
	    printf("FAILED: init_thread_params\n");
	    return demo_errno;
    };

    printf("** Nodes' status  **\n");
    printf("PID of main process: %d\n\n", getpid());
    printf(" - Currently at node %d\n", current_nid);


    for (i = 0; i < nodes; i++) {
        if (pnodes[i].status != POPCORN_NODE_ONLINE) continue;
        printf(" Node  %d: arch is %s\n", i,
               arch_sz[pnodes[i].arch + 1]);
    }

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
	//printf("\n");
	//for (i = 0; i < nthreads; i++) {
	//	printf("Thread %2d with tid [%d] created at %p on node %d\n\n", threads[i]->id, threads[i]->tid, threads[i], threads[i]->at); //[JMB] added "with tid"]
	//}

	/* Main loop */
	while (!finish) {
		int tid;
		int to;
		int from;
		int ret;

		printf("\n");
		if (!skip_print) {
			__print_thread_status();
		}
		skip_print = 0;

		printf("\nThread ITR : ");
		if(!(scanf("%d", &tid)))
        {
		    demo_errno = -4;
		    break;
        };


		if (tid == -1 || tid == 999) {
			break;
		}
		if (tid == 99) {
			continue;
		}
		if (tid == 88) {
			if((demo_errno = __print_ps()))
            {
			    break;
            };
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
            if(!(scanf("%d", &to)))
            {
                demo_errno = -4;
                break;
            };
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
		    demo_errno = -1;
		    break;
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

	switch(demo_errno)
    {
        case -1:
            printf("FAILED: migrate_thread with return value %d\n", ret);
            break;
        case -3:
            printf("FAILED: print_ps\n");
            break;
        case -4:
            printf("FAILED: scanf\n");
            break;
        case -EINVAL:
            printf("FAILED: popcorn_get_node_info");
            break;
        default:
            printf("PASSED\n");
            break;
    }
	return demo_errno;
}

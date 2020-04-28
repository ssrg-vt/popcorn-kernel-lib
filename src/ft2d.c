#define _GNU_SOURCE

// SPDX-License-Identifier: GPL-2.0-only, 3-clause BSD
/*
 * ft2d - Simple Migration Functional 2 Node Test D
 *
 *  Copyright (C) 2020 Narf Industries Inc., Javier Malave <javier.malave@narfindustries.com>
 *
 *
 * Based on code by:
 *   University of Virginia Tech - Popcorn Kernel Library
 *
 * What this file implements:
 *
 * Setup/Config Rules - Linux will be compiled and configured with
 * file kernel/popcorn/configs/config-x86_64-qemu.
 * IP addresses (10.4.4.100, 10.4.4.101) will be used for node 0 and node 1 respectively.
 * This test will run as part of Popcorn’s CI regression suite.
 *
 * Run Description - This test will fork multiple threads and migrate them using popcorn_migrate from node A to node B.
 * The test will only be performed on x86 architecture. Both nodes will be of the same architecture.
 * Pass criteria - Popcorn_migrate returns without errors. Threads are successfully migrated to Node B and back to Node A without errors.
 * Task_struct should match expected values at node B & node A.
 * Input/Output - This test takes three inputs. int A = Source Node; int B = Sink Node; int C = Number of Threads
 * Platform - This test must run on QEMU, HW (x86) -> [TODO]
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include "migrate.h"


#define FT2D_NODES 2
#define NODE_OFFLINE 0
#define X86_64_ARCH 1
static const char *arch_sz[] = {
	"unknown",
	"arm64",
	"x86-64",
	"ppc64le",
	NULL,
};

struct thread {
	pthread_t thread_info;
	pid_t tid;
	int source_nid;
	int sink_nid;
	int done;
	int err;
};
pthread_barrier_t barrier_start;
struct thread *pcn_thread;
struct thread **threads;
int nthreads;
int all_done;

static int __init_thread_params(void)
{
	int i;
	int init_errno;
	threads = (struct thread **)malloc(sizeof(struct thread *) * nthreads);


	for (i = 0; i < nthreads; i++) {
		if((init_errno = posix_memalign((void **)&(threads[i]), PAGE_SIZE, sizeof(struct thread)))) {
			return init_errno;
		};
	}

	pthread_barrier_init(&barrier_start, NULL, nthreads + 1);
	return 0;
}


int node_sanity_check(int local_nid, int remote_nid)
{
	int current_nid;
	struct popcorn_node_info pnodes[FT2D_NODES];
	int node_err = 0;

	/* Get Node Info. Make sure We can retrieve the right node's information */
	node_err = popcorn_get_node_info(&current_nid, pnodes, FT2D_NODES);

	if (node_err) {
		printf("FT_2_D FAILED: popcorn_get_node_info, Cannot retrieve the nodes' information at node %d. ERROR CODE %d\n", current_nid, node_err);
		return node_err;
	}

	/* Testing Node Info */
	if(current_nid != local_nid) {
		printf("FT_2_D FAILED: We should be at Node %d. Yet we are at node %d\n", local_nid, current_nid);
		node_err = -1;
		return node_err;
	} else if(pnodes[local_nid].status == NODE_OFFLINE) {
		printf("FT_2_D FAILED: Node %d is offline.\n", local_nid);
		node_err = -1;
		return node_err;
	} else if(pnodes[remote_nid].status == NODE_OFFLINE) {
		printf("FT_2_D FAILED: Node %d is offline.\n", remote_nid);
		node_err = -1;
		return node_err;
	} else if(pnodes[local_nid].arch != X86_64_ARCH) {
		printf("FT_2_D WARN: Node %d does not have expected X86 architecture. Architecture is %s.\n", local_nid, arch_sz[pnodes[local_nid].arch + 1]);
	} else if(pnodes[remote_nid].arch != X86_64_ARCH) {
		printf("FT_2_D WARN: Node %d does not have expected X86 architecture. Architecture is %s.\n", remote_nid, arch_sz[pnodes[remote_nid].arch + 1]);
	}

	return node_err;
}

int thread_sanity_check(int nid, pid_t tid)
{
	struct popcorn_thread_status status;
	int thread_err = 0;

	thread_err = popcorn_get_status(&status);

	if (thread_err) {
		printf("FT_2_D FAILED: popcorn_get_status, Cannot retrieve the thread' information at node %d. ERROR CODE: %d\n", nid, thread_err);
		return thread_err;
	}

	if(status.current_nid != nid) {
		printf("FT_2_D FAILED: popcorn_get_status, Thread %d should be at node %d. But instead it is at node %d\n", tid, nid, status.current_nid);
		thread_err = -1;
		return thread_err;
	}

	return thread_err;
}

static void *child_thread(void *input)
{
	struct thread *current_thread = input;
	pid_t source_tid = popcorn_gettid();
	pid_t sink_tid;
	pid_t temp_tid;
	pthread_barrier_wait(&barrier_start);

	if(source_tid == -1) {
		printf("FT_2_D FAILED: Thread ID is not a positive integer, TID: %d\n", source_tid);
		current_thread->err = -1;
		current_thread->done = 1;
		all_done++;
		return NULL;
	}

	current_thread->tid = source_tid;

	/* Node Sanity Check */
	if((current_thread->err = node_sanity_check(current_thread->source_nid, current_thread->sink_nid))) {
		current_thread->done = 1;
		all_done++;
		return NULL;
	}

	/*Get thread status. */
	if((current_thread->err = thread_sanity_check(current_thread->source_nid, source_tid))) {
		current_thread->done = 1;
		all_done++;
		return NULL;
	}

	/* Migrate Thread to sink_nid */
	current_thread->err = migrate(current_thread->sink_nid, NULL, NULL);
	if(current_thread->err) {
		switch (current_thread->err) {
			case -EINVAL:
				printf("FT_2_D FAILED: Thread %d. Invalid Migration Destination %d\n", source_tid, current_thread->sink_nid);
				current_thread->done = 1;
				break;
			case -EBUSY:
				printf("FT_2_D FAILED: Thread %d already running at destination %d\n", source_tid, current_thread->sink_nid);
				current_thread->done = 1;
				break;
			case -EAGAIN:
				printf("FT_2_D FAILED: Thread %d could not reach destination %d. Node is offline.\n", source_tid, current_thread->sink_nid);
				current_thread->done = 1;
				break;
			default:
				printf("FT_2_D FAILED: Thread %d could not migrate, process_server_do_migration returned %d\n", source_tid, current_thread->err);
				current_thread->done = 1;
		}
	}

	/* Did we fail at the switch statement? */
	if(current_thread->done) {
		all_done++;
		return NULL;
	}

	//printf("FT_2_D: We should have arrived at sink node.\n");

	/* We should be at sink_nid. Get TID at Sink Node */
	sink_tid = popcorn_gettid();

	if(sink_tid == -1) {
		//printf("FT_2_D FAILED: Thread ID is not a positive integer, TID: %d\n", sink_tid);
		current_thread->err = -1;
		current_thread->done = 1;
		all_done++;
		return NULL;
	}

	//printf("FT_2_D: Thread ID is %d\n", sink_tid);

	/* Node Sanity Check */
	if((current_thread->err = node_sanity_check(current_thread->sink_nid, current_thread->source_nid))) {
		current_thread->done = 1;
		all_done++;
		return NULL;
	}


	/* Migrate Thread Back to source_nid */
	current_thread->err = migrate(current_thread->source_nid, NULL, NULL);

	printf("FT_2_D: We should have arrived back at source node.\n");

	/* We should be at sink_nid. Get TID at Sink Node */
	temp_tid = popcorn_gettid();

	if(temp_tid == -1) {
		printf("FT_2_D FAILED: Thread ID is not a positive integer, TID: %d\n", temp_tid);
		current_thread->err = -1;
		current_thread->done = 1;
		all_done++;
		return NULL;
	} else if(temp_tid != source_tid) {
		printf("FT_2_D FAILED: Thread ID %d does not match original TID %d\n", temp_tid, source_tid);
		current_thread->err = -1;
		current_thread->done = 1;
		return NULL;
	}

	printf("FT_2_D: Thread ID is %d\n", source_tid);

	/* Node Sanity Check */
	if((current_thread->err = node_sanity_check(current_thread->source_nid, current_thread->sink_nid)))
	{
		current_thread->done = 1;
		all_done++;
		return NULL;
	}

	printf("FT_2_D Thread %d PASSED at NODE %d\n", source_tid, current_thread->source_nid);
	current_thread->done = 1;
	all_done++;
	return NULL;
}

int main(int argc, char *argv[])
{

#ifdef __x86_64__
    int ft2d_errno = 0;
    int source_node;
    int sink_node;
    int i;
    pid_t source_pid = popcorn_gettid();
    nthreads = 0;
    all_done = 0;

    if(source_pid == -1) {
        printf("FT_2_D FAILED: Process ID is not a positive integer, PID: %d\n", source_pid); // NOTE TID (TGID) should be == to PID (TID) in this instance
        ft2d_errno = -1;
        return ft2d_errno;
    }

	if (argc != 4) {
        printf("FT_2_D FAILED: This test takes 3 arguments, Source Node ID, Sink Node ID, # of threads.\n");
    }


	printf("FT_2_D: Process ID is %d\n", source_pid);

	source_node = atoi(argv[1]);
	sink_node = atoi(argv[2]);
    	nthreads = atoi(argv[3]);

    if(source_node == sink_node) {
        printf("FT_2_D FAILED: Source Node ID must be different to Sink Node ID\n");
        ft2d_errno = -1;
        return ft2d_errno;
    } else if((source_node | sink_node) < 0 || source_node >= MAX_POPCORN_NODES || sink_node >= MAX_POPCORN_NODES) {
        printf("FT_2_D FAILED: Node ID's must be a positive integer 0-31\n");
        ft2d_errno = -1;
        return ft2d_errno;
    }

    /* Init popcorn "traveling" threads */
    if((ft2d_errno = __init_thread_params())){
        printf("FT_2_D FAILED: __init_thread_params() failed error %d\n", ft2d_errno);
        return ft2d_errno;
    }
    for(i = 0; i < nthreads; i++) {
        pcn_thread = threads[i];
        pcn_thread->tid = 0;
        pcn_thread->done = 0;
        pcn_thread->err = 0;
        pcn_thread->source_nid = source_node;
        pcn_thread->sink_nid = sink_node;
        /* Create and init pthread */
        pthread_create(&pcn_thread->thread_info, NULL, &child_thread, pcn_thread);
    }

    pthread_barrier_wait(&barrier_start);

    while(all_done < nthreads);

    for(i = 0; i < nthreads; i++) {
	int *errno_ptr = &ft2d_errno;
	pcn_thread = threads[i];
    	pthread_join(pcn_thread->thread_info, (void(**))(&errno_ptr));
    	printf("FT_2_D TEST at NODE %d Thread %d exited with CODE %d\n", source_node, pcn_thread->tid, pcn_thread->err);
    	free(pcn_thread);
    }

    printf("FT_2_D TEST PASSED at NODE %d\n", source_node);
    return ft2d_errno;
#else
	int ft2d_errno;
	printf("FT_2_D: Test only supports X86_64 Architecture\n");
	ft2d_errno = -1;
	return ft2d_errno;
#endif
}

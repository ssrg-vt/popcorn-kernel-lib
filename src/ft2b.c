#define _GNU_SOURCE

// SPDX-License-Identifier: GPL-2.0-only
/*
 * ft2b - Simple Migration Functional 2 Node Test B
 *
 *  Copyright (C) 2020 Narf Industries Inc., Javier Malave <javier.malave@narfindustries.com>
 *
 *
 * Based on code by:
 *   University of Virginia Tech - Popcorn Kernel Library
 *   [First Name, Last Name] <TBD>
 *
 * What this file implements:
 *
 * Setup/Config Rules - Linux will be compiled and configured with
 * file popcorn-kernel/kernel/popcorn/configs/config-x86_64-qemu.
 * IP addresses (10.4.4.100, 10.4.4.101) will be used for node 0 and node 1 respectively.
 * This test will run as part of Popcorn’s CI regression suite.
 *
 * Run Description - This test will call from the parent process popcorn_migrate to migrate a single task from node A to node B.
 * The test will only be performed on x86 architecture. Both nodes will be of the same architecture.
 * Pass criteria - Popcorn_migrate returns without errors. Task is successfully migrated to node A without errors or segmentation faults,
 * kernel panics or oops. Task_struct should match expected values at node B.
 * Input/Output - This test takes two inputs. int A = Source Node; int B = Sink Node
 * Platform - This test must run on QEMU, and HW (x86)
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#include "migrate.h"

#define FT2B_NODES 2
#define NODE_OFFLINE 0
#define X86_64_ARCH 1
#define FT2B_ERR    125
static const char *arch_sz[] = {
        "unknown",
        "arm64",
        "x86-64",
        "ppc64le",
        NULL,
};

int callback_arguments;
void *callback_ptr;
void callback_ft2b(void *args)
{
    return;
}
void init_callback_arguments()
{
    callback_arguments = 0;
    callback_ptr = (void *)&callback_arguments;
    return;
}

int main(int argc, char *argv[])
{

#ifdef __x86_64__
    struct popcorn_node_info pnodes[FT2B_NODES];
    struct popcorn_thread_status status;
	int current_nid;
	int ft2b_errno = 0;
	int source_node;
	int sink_node;
    pid_t source_tid = popcorn_gettid();
    pid_t sink_tid;
    pid_t temp_tid;

    // Init Callback Arguments
    init_callback_arguments();

    if(source_tid == -1)
    {
        printf("FT_2_B FAILED: Process ID is not a positive integer, PID: %d\n", source_tid); // NOTE TID (TGID) should be == to PID (TID) in this instance
        ft2b_errno = -FT2B_ERR;
        return ft2b_errno;
    }

	if (argc != 3)
    {
        printf("FT_2_B FAILED: This test takes 2 arguments, Source Node ID, Sink Node ID\n");
    }


	printf("FT_2_B: Process ID is %d\n", source_tid);

	source_node = atoi(argv[1]);
    sink_node = atoi(argv[2]);

    if(source_node == sink_node)
    {
        printf("FT_2_B FAILED: Source Node ID must be different to Sink Node ID\n");
        ft2b_errno = -(FT2B_ERR+1);
        return ft2b_errno;
    }
    else if((source_node | sink_node) < 0 || source_node >= MAX_POPCORN_NODES || sink_node >= MAX_POPCORN_NODES)
    {
        printf("FT_2_B FAILED: Node ID's must be a positive integer 0-31\n");
        ft2b_errno = -(FT2B_ERR+2);
        return ft2b_errno;
    }


	/* Get Node Info. Make sure We can retrieve the right node's information */
    ft2b_errno = popcorn_get_node_info(&current_nid, pnodes, FT2B_NODES);

    if (ft2b_errno)
    {
        printf("FT_2_B FAILED: popcorn_get_node_info, Cannot retrieve the nodes' information at node %d. ERROR CODE %d\n", current_nid, ft2b_errno);
        return ft2b_errno;
    }

    /* Testing Node Info */
    if(current_nid != source_node)
    {
        printf("FT_2_B FAILED: We should be at Node %d. Yet we are at node %d\n", source_node, current_nid);
        ft2b_errno = -(FT2B_ERR+3);
        return ft2b_errno;
    }
    else if(pnodes[source_node].status == NODE_OFFLINE)
    {
        printf("FT_2_B FAILED: Node %d is offline.\n", source_node);
        ft2b_errno = -(FT2B_ERR+4);
        return ft2b_errno;
    }
    else if(pnodes[sink_node].status == NODE_OFFLINE)
    {
        printf("FT_2_B FAILED: Node %d is offline.\n", sink_node);
        ft2b_errno = -(FT2B_ERR+5);
        return ft2b_errno;
    }
    else if(pnodes[source_node].arch != X86_64_ARCH)
    {
        printf("FT_2_B WARN: Node %d does not have expected X86 architecture. Architecture is %s.\n", source_node, arch_sz[pnodes[source_node].arch + 1]);
    }
    else if(pnodes[sink_node].arch != X86_64_ARCH)
    {
        printf("FT_2_B WARN: Node %d does not have expected X86 architecture. Architecture is %s.\n", sink_node, arch_sz[pnodes[source_node].arch + 1]);
    }

    /*Get process status. */
    ft2b_errno = popcorn_get_status(&status);

    if (ft2b_errno)
    {
        printf("FT_2_B FAILED: popcorn_get_status, Cannot retrieve the process' information at node %d. ERROR CODE: %d\n", source_node, ft2b_errno);
        return ft2b_errno;
    }

    if(status.current_nid != source_node)
    {
        printf("FT_2_B FAILED: popcorn_get_status, Process %d should still be at node %d. But instead it is at node %d\n", source_tid, source_node, status.current_nid);
        ft2b_errno = -(FT2B_ERR+6);
        return ft2b_errno;
    }

    /* TODO: Need to understand what peer_nid, peer_pid and proposed_nid mean in this context to test for the right values */

    /* Migrate Process to Sink_Node */
    ft2b_errno = migrate(sink_node, (void*)&callback_ft2b, callback_ptr);
    if(ft2b_errno)
    {
        switch (ft2b_errno) {
            case -EINVAL:
                printf("FT_2_B FAILED: Process %d. Invalid Migration Destination %d\n", source_tid, sink_node);
                return ft2b_errno;;
            case -EBUSY:
                printf("FT_2_B FAILED: Process %d already running at destination %d\n", source_tid, sink_node);
                return ft2b_errno;;
            case -EAGAIN:
                printf("FT_2_B FAILED: Process %d could not reach destination %d. Node is offline.\n", source_tid, sink_node);
                return ft2b_errno;;
            default:
                printf("FT_2_B FAILED: Process %d could not migrate, process_server_do_migration returned %d\n", source_tid, ft2b_errno);
                return ft2b_errno;;
        }
    }

    //printf("FT_2_B: We should have arrived at sink node.\n"); // Access to STDOUT or other file descriptors not currently implemented for remote threads.

    /* We should be at sink_node. Get PID at Sink Node */
    sink_tid = popcorn_gettid();

    if(sink_tid == -1)
    {
        //printf("FT_2_B FAILED: Process ID is not a positive integer, PID: %d\n", sink_tid); // NOTE TID (TGID) should be == to PID (TID) in this instance
        ft2b_errno = -(FT2B_ERR+7);
        return ft2b_errno;
    }

    //printf("FT_2_B: Process ID is %d\n", sink_tid); // Access to STDOUT or other file descriptors not currently implemented for remote threads.

    /* Get Node Info. Make sure We can retrieve the right node's information */
    ft2b_errno = popcorn_get_node_info(&current_nid, pnodes, FT2B_NODES);

    if (ft2b_errno)
    {
        //printf("FT_2_B FAILED: popcorn_get_node_info, Cannot retrieve the nodes' information at node %d. ERROR CODE %d\n", current_nid, ft2b_errno);
        return ft2b_errno;
    }

    if(current_nid != sink_node)
    {
        //printf("FT_2_B FAILED: We should be at Node %d. Yet we are at node %d\n", sink_node, current_nid);
        ft2b_errno = -(FT2B_ERR+8);
        return ft2b_errno;
    }

    /* Migrate Process Back to Source_Node */
    ft2b_errno = migrate(source_node, (void*)&callback_ft2b, callback_ptr);

    printf("FT_2_B: We should have arrived back at source node.\n");

    /* We should be at sink_node. Get PID at Sink Node */
    temp_tid = popcorn_gettid();

    if(temp_tid == -1)
    {
        printf("FT_2_B FAILED: Process ID is not a positive integer, PID: %d\n", temp_tid); // NOTE TID (TGID) should be == to PID (TID) in this instance
        ft2b_errno = -(FT2B_ERR+9);
        return ft2b_errno;
    }
    else if(temp_tid != source_tid)
    {
        printf("FT_2_B FAILED: Process ID %d does not match original PID %d\n", temp_tid, source_tid); // NOTE TID (TGID) should be == to PID (TID) in this instance
        ft2b_errno = -(FT2B_ERR+10);
        return ft2b_errno;
    }

    printf("FT_2_B: Process ID is %d\n", temp_tid);

    /* Get Node Info. Make sure We can retrieve the right node's information */
    ft2b_errno = popcorn_get_node_info(&current_nid, pnodes, FT2B_NODES);

    if (ft2b_errno)
    {
        printf("FT_2_B FAILED: popcorn_get_node_info, Cannot retrieve the nodes' information at node %d. ERROR CODE %d\n", current_nid, ft2b_errno);
        return ft2b_errno;
    }

    if(current_nid != source_node)
    {
        printf("FT_2_B FAILED: We should be at Node %d. Yet we are at node %d\n", source_node, current_nid);
        ft2b_errno = -(FT2B_ERR+11);
        return ft2b_errno;
    }

    printf("FT_2_B PASSED at NODE %d with error code %d\n", current_nid, ft2b_errno);
	return ft2b_errno;
#else
    int ft2b_errno;
    printf("FT_2_B: Test only supports X86_64 Architecture\n");
    ft2b_errno = -FT2B_ERR;
    return ft2b_errno;
#endif
}

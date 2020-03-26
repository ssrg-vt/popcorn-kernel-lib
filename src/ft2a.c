#define _GNU_SOURCE

// SPDX-License-Identifier: GPL-2.0-only
/*
 * ft2a - Simple Migration Functional 2 Node Test A
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

#define FT2A_NODES 2
#define NODE_OFFLINE 0
#define X86_64_ARCH 1
#define FT2A_ERR    125
static const char *arch_sz[] = {
        "unknown",
        "arm64",
        "x86-64",
        "ppc64le",
        NULL,
};

int main(int argc, char *argv[]) {

#ifdef __x86_64__
    struct popcorn_node_info pnodes[FT2A_NODES];
    struct popcorn_thread_status status;
    int current_nid;
    int f2ta_errno = 0;
    int source_node;
    int sink_node;
    pid_t tid = popcorn_gettid();

    if(tid == -1)
    {
        printf("FT_2_A FAILED: Process ID is not a positive integer, PID: %d\n", tid); // NOTE TID (TGID) should be == to PID (TID) in this instance
        f2ta_errno = -FT2A_ERR;
        return f2ta_errno;
    }

    if (argc != 3)
    {
        printf("FT_2_A FAILED: This test takes 2 arguments, Source Node ID, Sink Node ID\n");
    }


    printf("FT_2_A: Process ID is %d\n", tid);

    source_node = atoi(argv[1]);
    sink_node = atoi(argv[2]);

    if(source_node == sink_node)
    {
        printf("FT_2_A FAILED: Source Node ID must be different to Sink Node ID\n");
        f2ta_errno = -(FT2A_ERR+1);
        return f2ta_errno;
    }
    else if((source_node | sink_node) < 0 || source_node >= MAX_POPCORN_NODES || sink_node >= MAX_POPCORN_NODES)
    {
        printf("FT_2_A FAILED: Node ID's must be a positive integer 0-31\n");
        f2ta_errno = -(FT2A_ERR+2);
        return f2ta_errno;
    }

    /* Get Node Info. Make sure We can retrieve the right node's information */
    f2ta_errno = popcorn_get_node_info(&current_nid, pnodes, FT2A_NODES);

    if (f2ta_errno)
    {
        printf("FT_2_A FAILED: popcorn_get_node_info, Cannot retrieve the nodes' information at node %d. ERROR CODE %d\n", current_nid, f2ta_errno);
        return f2ta_errno;
    }

    /* Testing Node Info */
    if(current_nid != source_node)
    {
        printf("FT_2_A FAILED: We should be at Node %d. Yet we are at node %d\n", source_node, current_nid);
        f2ta_errno = -(FT2A_ERR+3);
        return f2ta_errno;
    }
    else if(pnodes[source_node].status == NODE_OFFLINE)
    {
        printf("FT_2_A FAILED: Node %d is offline.\n", source_node);
        f2ta_errno = -(FT2A_ERR+4);
        return f2ta_errno;
    }
    else if(pnodes[sink_node].status == NODE_OFFLINE)
    {
        printf("FT_2_A FAILED: Node %d is offline.\n", sink_node);
        f2ta_errno = -(FT2A_ERR+5);
        return f2ta_errno;
    }
    else if(pnodes[source_node].arch != X86_64_ARCH)
    {
        printf("FT_2_A WARN: Node %d does not have expected X86 architecture. Architecture is %s.\n", source_node, arch_sz[pnodes[source_node].arch + 1]);
    }
    else if(pnodes[sink_node].arch != X86_64_ARCH)
    {
        printf("FT_2_A WARN: Node %d does not have expected X86 architecture. Architecture is %s.\n", sink_node, arch_sz[pnodes[source_node].arch + 1]);
    }

    /*Get process status. */
    f2ta_errno = popcorn_get_status(&status);

    if (f2ta_errno)
    {
        printf("FT_2_A FAILED: popcorn_get_status, Cannot retrieve the process' information at node %d. ERROR CODE: %d\n", source_node, f2ta_errno);
        return f2ta_errno;
    }

    if(status.current_nid != source_node)
    {
        printf("FT_2_A FAILED: popcorn_get_status, Process %d should still be at node %d. But instead it is at node %d\n", tid, source_node, status.current_nid);
        f2ta_errno = -(FT2A_ERR+6);
        return f2ta_errno;
    }

    /* TODO: Need to understand what peer_nid, peer_pid and proposed_nid mean in this context to test for the right values */

    /* Migrate Process to Sink_Node */
    f2ta_errno = migrate(sink_node, NULL, NULL);

    if(f2ta_errno)
    {
        switch (f2ta_errno) {
            case -EINVAL:
                printf("FT_2_A FAILED: Process %d. Invalid Migration Destination %d\n", tid, sink_node);
                return f2ta_errno;
            case -EBUSY:
                printf("FT_2_A FAILED: Process %d already running at destination %d\n", tid, sink_node);
                return f2ta_errno;
            case -EAGAIN:
                printf("FT_2_A FAILED: Process %d could not reach destination %d. Node is offline.\n", tid, sink_node);
                return f2ta_errno;
            default:
                printf("FT_2_A FAILED: Process %d could not migrate, process_server_do_migration returned %d\n", tid, f2ta_errno);
                return f2ta_errno;
        }
    }

    //printf("FT_2_A: We should have arrived at sink node.\n"); // Access to STDOUT or other file descriptors not currently implemented for remote threads.

    /* We should be at sink_node. Get PID at Sink Node */
    tid = popcorn_gettid();

    if(tid == -1)
    {
        //printf("FT_2_A FAILED: Process ID is not a positive integer, PID: %d\n", tid); // NOTE TID (TGID) should be == to PID (TID) in this instance
        f2ta_errno = -(FT2A_ERR+7);
        return f2ta_errno;
    }

    //printf("FT_2_A: Process ID is %d\n", tid);

    /* Get Node Info. Make sure We can retrieve the right node's information */
    f2ta_errno = popcorn_get_node_info(&current_nid, pnodes, FT2A_NODES);

    if (f2ta_errno)
    {
       // printf("FT_2_A FAILED: popcorn_get_node_info, Cannot retrieve the nodes' information at node %d. ERROR CODE %d\n", current_nid, f2ta_errno);
        return f2ta_errno;
    }

    if(current_nid != sink_node)
    {
        //printf("FT_2_A FAILED: We should be at Node %d. Yet we are at node %d\n", sink_node, current_nid);
        f2ta_errno = -(FT2A_ERR+8);
        return f2ta_errno;
    }


    //printf("FT_2_A PASSED at NODE %d\n", current_nid);
    return f2ta_errno;
#else
    int f2ta_errno;
    printf("FT_2_A: Test only supports X86_64 Architecture\n");
    f2ta_errno = -FT2A_ERR;
    return f2ta_errno;
#endif
}

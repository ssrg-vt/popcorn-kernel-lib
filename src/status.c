#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>

#include "migrate.h"

static const char *arch_sz[] = {
	"unknown",
	"arm64",
	"x86-64",
	"ppc64le",
	NULL,
};

int main(int argc, const char *argv[])
{
	struct popcorn_thread_status status;
	struct popcorn_node_info nodes[MAX_POPCORN_NODES];
	int i;
	int current_nid;
	int dest;

	/* Inquiry the entire rack's status */
	printf("** Nodes' status  **\n");
	if (popcorn_get_node_info(&current_nid, nodes)) {
		perror(" Cannot retrieve the nodes' information");
		return -EINVAL;
	}
	printf(" - Currently at node %d\n", current_nid);
	for (i = 0; i < MAX_POPCORN_NODES; i++) {
		if (nodes[i].status != POPCORN_NODE_ONLINE) continue;
		printf("  %d: %s\n", i, 
				arch_sz[nodes[i].arch + 1]);
	}

	/* Query the current thread's migration status */
	printf("\n** Current thread status **\n");
	if (popcorn_get_status(&status)) {
		perror(" Cannot retrieve the current thread's status");
		return -EINVAL;
	}
	printf(" - currently at node %d\n", status.current_nid);
	printf(" - migration destination is ");
	if (status.proposed_nid == -1) {
		printf("not proposed\n");
	} else {
		printf("proposed to %d\n", status.proposed_nid);
	}

	srand(time(NULL));
	dest = rand() % MAX_POPCORN_NODES;

	/* Propose to migrate this thread to 1 */
	printf("\n** Migration proposal test **\n");
	printf(" - Propose to migrate this to node %d\n", dest);
	popcorn_propose_migration(0, dest);

	popcorn_get_status(&status);
	printf(" - migration destination is ");
	if (status.proposed_nid == -1) {
		printf("not proposed\n");
	} else {
		printf("proposed to %d (%s)\n", status.proposed_nid,
			dest == status.proposed_nid ? "PASSED" : "FAILED");
	}

	return 0;
}

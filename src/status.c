#include <stdio.h>
#include <errno.h>

#include "migrate.h"

static const char *arch_sz[] = {
	"arm64",
	"x86-64",
	"ppc64le",
	"unknown",
	NULL,
};

int main(int argc, const char *argv[])
{
	struct popcorn_thread_status status;
	struct popcorn_node_info nodes[MAX_POPCORN_NODES];
	int i;

	/* Inquiry the entire rack's status */
	printf("** Nodes' status  **\n");
	if (popcorn_get_node_info(nodes)) {
		perror(" Cannot retrieve the nodes' information");
		return -EINVAL;
	}
	for (i = 0; i < MAX_POPCORN_NODES; i++) {
		if (nodes[i].status != POPCORN_NODE_ONLINE) continue;
		printf(" %d: %s\n", i, 
				arch_sz[nodes[i].arch]);
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


	/* Propose to migrate this thread to 1 */
	printf("\n** Migration proposal test **\n");
	printf("  - Propose to migrate this to node 1\n");
	popcorn_propose_migration(0, 1);

	popcorn_get_status(&status);
	printf(" - migration destination is ");
	if (status.proposed_nid == -1) {
		printf("not proposed\n");
	} else {
		printf("proposed to %d\n", status.proposed_nid);
	}

	return 0;
}

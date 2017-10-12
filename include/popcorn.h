#ifndef _POPCORN_H_
#define _POPCORN_H_

#include <stdlib.h>
#ifdef _OPENMP
#include <omp.h>
#endif

/**
 * Migrate this thread to the node nid. if nid == -1, use the proposed
 * migration location.
 */
void migrate(int nid, void (*callback)(void *), void *callback_param);


/*
 * Check wheter the migration is proposed by others such as a scheduler
 *
 * return >0 if the migration is proposed.
 * return 0 otherwise.
 */
int popcorn_migration_proposed(void);


/**
 * Propose to migrate pid to nid
 *
 * return 0 success, return non-zero otherwise.
 */
int popcorn_propose_migration(int tid, int nid);


/**
 * Allocate memory aligned to the page boundary
 */
void *popcorn_malloc(int size);


/**
 * Return the tid of the current context. This is a wrapper for
 * syscall(SYS_gettid)
 */
int popcorn_gettid();


enum popcorn_node_status {
	POPCORN_NODE_OFFLINE = 0x00,
	POPCORN_NODE_ONLINE = 0x01,
};

enum popcorn_arch_types {
	POPCORN_NODE_AARCH64 = 0,
	POPCORN_NODE_PPC64 = 1,
	POPCORN_NODE_X86 = 2,
	POPCORN_NODE_UNKNOWN,
};

struct popcorn_node_info {
	unsigned int status;
	int arch;
	int distance;
};

/**
 * Get the popcorn node information
 */
int popcorn_get_node_info(int nid, struct popcorn_node_info *info);


#ifdef _OPENMP
/**
 * Helper functions to transform omp "parallel for" to "parallel"
 * Example:
 *
 *   #ifndef _POPCORN
 *   #pragma omp parallel for default(shared) private(i)
 *   for (i = 0; i <= 10; i++) {
 *   #else
 *   #pragma omp prallel default(shared)
 *   {
 *   int i;
 *   POPCORN_OMP_SPLIT_START(i, 0, 10, THREADS_PER_CORE);
 *   #endif
 *         ...
 *   #ifdef _POPCORN
 *   POPCORN_OMP_SPLIT_END();
 *   }
 *   #endif
 */
int popcorn_omp_split(int tid, int threads,
		int start, int end, int *range_start, int *range_end);

#define POPCORN_OMP_SPLIT_START(k, start, end, N) \
{	\
	int _s, _e; \
	const int _tid = omp_get_thread_num(); \
	const int _cores = N; \
	popcorn_omp_split(_tid, omp_get_max_threads(), (start), (end), &_s, &_e); \
	/* printf("%d : %d-%d : %d-%d\n", _tid, start, end, _s, _e); */ \
	if (_tid / _cores) migrate(_tid / _cores, NULL, NULL); \
	for ((k) = _s; (k) <= _e; (k)++)

#define POPCORN_OMP_SPLIT_END() \
	if (_tid / _cores) migrate(0, NULL, NULL); \
}
#endif
#endif

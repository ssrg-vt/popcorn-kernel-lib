#ifndef _POPCORN_H_
#define _POPCORN_H_

#include <stdlib.h>

#define MAX_POPCORN_NODES	32


/**
 * Migrate this thread to the node nid. if nid == -1, use the proposed
 * migration location.
 *
 * return 0 on success, return non-zero otherwise
 *  EINVAL: invalid migration destination @nid
 *  EAGAIN: @nid is offline now
 *  EBUSY:  already running at @nid
 */
void migrate(int nid, void (*callback)(void *), void *callback_param);


void migrate_schedule(size_t region,
                      int popcorn_tid,
                      void (*callback)(void *),
                      void *callback_data);

/*
 * Get the current status of the current thread regarding migration
 *
 * return 0 on fetching the status successfully.
 * return non-zero otherwise.
 *  EINVAL: invalid @status
 */
struct popcorn_thread_status {
	int current_nid;
	int proposed_nid;
	int peer_nid;
	int peer_pid;
};

int popcorn_get_status(struct popcorn_thread_status *status);


static inline int popcorn_current_nid(void)
{
	struct popcorn_thread_status status;
	if (popcorn_get_status(&status)) return -1;

	return status.current_nid;
}

/**
 * Propose to migrate @tid to @nid
 *
 * If @tid 0 means to propose the migration for the current one.
 *
 * return 0 on success, return non-zero otherwise.
 *  ENOENT: no thread corresponding to @tid
 *  EINVAL: invalid @nid
 */
int popcorn_propose_migration(int tid, int nid);


/**
 * Allocate memory aligned to the page boundary
 */
#include <malloc.h>
static inline void *popcorn_malloc(size_t size)
{
	void *p = NULL;
	posix_memalign(&p, 4096, size);
	return p;
}


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
	POPCORN_NODE_X86 = 1,
	POPCORN_NODE_PPC64 = 2,
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
int popcorn_get_node_info(struct popcorn_node_info *info);


#ifdef _OPENMP
#include <omp.h>

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
static inline void popcorn_omp_split(int tid, int threads,
		int start, int end, int *range_start, int *range_end)
{
	int N = end - start + 1;
	float step;
	if (N < threads) {
		threads = N;
		if (tid >= N) {
			*range_start = 0;
			*range_end = -1;
			return;
		}
	}
	step = (float)N / threads;
	*range_start = start + tid * step;
	if (tid != threads - 1) {
		*range_end = start + (tid + 1) * step - 1;
	} else {
		*range_end = end;
	}
	/*
	printf("%d : %d - %d, %d\n", tid, *range_start, *range_end,
			*range_end - *range_start + 1);
	*/
}


#if 0
#define POPCORN_OMP_SPLIT_START(k, start, end, N) \
{	\
	int _s, _e; \
	const int _tid = omp_get_popcorn_tid(); \
	popcorn_omp_split(_tid, omp_get_max_threads(), (start), (end), &_s, &_e); \
	migrate_schedule(0, _tid, NULL, NULL); \
	popcorn_omp_id(R); \
	for ((k) = _s; (k) <= _e; (k)++)


#define POPCORN_OMP_SPLIT_END() \
	popcorn_omp_id(0);
	migrate(0, NULL, NULL); \
}

#else
#define POPCORN_OMP_SPLIT_START(k, start, end, N) \
{	\
	int _s, _e; \
	const int _tid = omp_get_thread_num(); \
	const int _cores = N; \
	popcorn_omp_split(_tid, omp_get_max_threads(), (start), (end), &_s, &_e); \
	if (_tid / _cores) migrate(_tid / _cores, NULL, NULL); \
	for ((k) = _s; (k) <= _e; (k)++)


#define POPCORN_OMP_SPLIT_END() \
	if (_tid / _cores) migrate(0, NULL, NULL); \
}
#endif

#endif /* _OPENMP */

#endif

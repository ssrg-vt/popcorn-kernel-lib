#ifndef _POPCORN_H_
#define _POPCORN_H_

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
 * Helper functions to transform omp "parallel for" to "parallel"
 * Example:
 *
 *   #ifndef _POPCORN
 *   #pragma omp parallel for
 *   for (i = 0; i <= 10; i++) {
 *   #else
 *   {
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
	if (_tid / _cores) migrate(_tid / _cores, NULL, NULL); \
	for ((k) = _s; (k) <= _e; (k)++) {

#define POPCORN_OMP_SPLIT_END() \
	if (_tid / _cores) migrate(0, NULL, NULL); \
}
#endif

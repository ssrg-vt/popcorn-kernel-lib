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

#endif

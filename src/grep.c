/**
 * grep: search for the @keys_raw from the file @fdata_name.
 */ 

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <getopt.h>
#include <errno.h>
#include <ctype.h>

#include "migrate.h"
#include "utils.h"

int num_nodes = 1;
int threads_per_node = 8;
const char *fdata_name = NULL;

#define MAX_REC_LEN 512
#define OFFSET 5

typedef struct {
	int tid;
	char *buffer;
	char *buffer_final;
	char *data;
	unsigned long length;
#ifdef _ALIGN_VARIABLES
	char padding[PAGE_SIZE 
			- sizeof(int) * 1
			- sizeof(char *) * 3
			- sizeof(unsigned long)];
#endif
} ALIGN_TO_PAGE targ_t;


/* Keys to match against */
#define NR_KEYS 4
char *keys_raw[NR_KEYS] = {
	"Helloworld",
	"howareyou",
	"ferrari",
	"whotheman",
};

struct {
	char *keys_final[NR_KEYS];
#ifdef _ALIGN_VARIABLES
	char padding[PAGE_SIZE - sizeof(char *) * NR_KEYS];
#endif
} keys;

/* Final result */
int key_matches[NR_KEYS] = {0};

static int get_word(char *str, char *word, int length)
{
    int i, so_far = 0;

    // Skip leading spaces
    for (i = 0; i < length; i++) {
        if (str[i] == ' ' || str[i] == '\n' || str[i] == '\r') continue;
        break;
    }

    for (; i < length && so_far < MAX_REC_LEN - 1; i++) {
        if (str[i] == ' ' || str[i] == '\n' || str[i] == '\r') {
            *word = '\0';
            break;
        }
        *word++ = str[i];
        so_far++;
    }

    if (so_far == MAX_REC_LEN - 1) {
        *word = '\0';
    }

    return i;
}

/**
 * compute_hashes()
 * Simple Cipher to generate a hash of the word 
 */
static void compute_hashes(char* word, char* final_word)
{
    size_t i;
    for (i = 0; i< strlen(word); i++) {
        final_word[i] = toupper(word[i]) + OFFSET;
    }
    final_word[i] = '\0';
}

/**
 * string_match_worker()
 * Map Function that checks the hash of each word to the given hashes
 */
static void *string_match_thread(void *args)
{
    assert(args);
    
    targ_t* targ = (targ_t*)args;
    size_t length = targ->length;
	size_t total_len = 0;
	int i;

    int key_len;
    char *file_data = targ->data;
#ifndef _ALIGN_VARIABLES
    char *cur_word = targ->buffer;
    char *cur_word_final = targ->buffer_final;
#else
    char cur_word[MAX_REC_LEN];
    char cur_word_final[MAX_REC_LEN];
	int local_matches[NR_KEYS] = {0};
#endif

    popcorn_migrate(targ->tid / threads_per_node);
    bzero(cur_word, MAX_REC_LEN);
    bzero(cur_word_final, MAX_REC_LEN);

    while (total_len < length) {
        key_len = get_word(file_data, cur_word, length - total_len);

        compute_hashes(cur_word, cur_word_final);

		for (i = 0;i < NR_KEYS; i++) {
#ifndef _ALIGN_VARIABLES
			if(!strcmp(keys.keys_final[i], cur_word_final)) key_matches[i]++;
#else
			if(!strcmp(keys.keys_final[i], cur_word_final)) local_matches[i]++;
#endif
		}

        file_data += key_len;
        bzero(cur_word,MAX_REC_LEN);
        bzero(cur_word_final, MAX_REC_LEN);
        total_len+=key_len;
    }
    popcorn_migrate(0);

#ifdef _ALIGN_VARIABLES
	for (i = 0; i < NR_KEYS; i++) {
		key_matches[i] += local_matches[i];
	}
#else
	free(cur_word);
	free(cur_word_final);
#endif
    return (void *)0;
}


/**
 * string_match_splitter()
 * Splitter Function to assign portions of the file to each thread
 */
static void string_match(char *data, const size_t data_length)
{
    pthread_attr_t attr;
    pthread_t * tids;
	size_t data_completed = 0;
    int i, num_procs;
#ifdef _ALIGN_VARIABLES
	char *buf = popcorn_malloc(PAGE_SIZE);
#endif

	for (i = 0; i < NR_KEYS; i++) {
		key_matches[i] = 0;
#ifndef _ALIGN_VARIABLES
		keys.keys_final[i] = malloc(strlen(keys_raw[i]));
#else
		keys.keys_final[i] = buf;
		buf += strlen(keys_raw[i]) + 1;
#endif
		compute_hashes(keys_raw[i], keys.keys_final[i]);
	}

    num_procs = num_nodes * threads_per_node;
    PRINTF("The number of processors is %d\n", num_procs);

    tids = (pthread_t *)malloc(num_procs * sizeof(pthread_t));

    /* Thread must be scheduled systemwide */
    pthread_attr_init(&attr);
    pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);

    unsigned long req_bytes = data_length / num_procs;

    targ_t *targs = popcorn_malloc(sizeof(targ_t) * num_procs);

    for (i = 0; i < num_procs; i++) {
		targ_t *targ = targs + i;

		memset(targ, 0x00, sizeof(*targ));
        targ->tid = i;
#ifndef _ALIGN_VARIABLES
		targ->buffer = (char *)malloc(MAX_REC_LEN);
		targ->buffer_final = (char *)malloc(MAX_REC_LEN);
#endif
                
        /* Assign the required number of bytes */        
        size_t available_bytes = data_length - data_completed;
        if (available_bytes < 0)
			available_bytes = 0;

        targ->data = data + data_completed;
        targ->length = (req_bytes < available_bytes)? req_bytes:available_bytes;

        char* final_ptr = targ->data + targ->length;
		size_t counter = data_completed + targ->length;

         /* make sure we end at a word */
        while (counter < data_length && 
				*final_ptr != '\n' && *final_ptr != '\r' &&
				*final_ptr != '\0' && *final_ptr != ' ')
        {
            counter++;
            final_ptr++;
        }

		/* Skip non-word characters */
		while (counter < data_length && 
				(*final_ptr == '\r' || *final_ptr == '\n' ||
				 *final_ptr == '\0' || *final_ptr == ' ')) {
			counter++;
			final_ptr++;
		}

        targ->length = counter - data_completed;
        data_completed = counter;
        pthread_create(&tids[i], &attr, string_match_thread, targ);
    }

    /* Barrier, wait for all threads to finish */
    for (i = 0; i < num_procs; i++) {
        unsigned long long ret_val = 0;
        pthread_join(tids[i], (void **)(void*)&ret_val);
    }

	PRINTF("grep: results\n");
	for (i = 0; i < NR_KEYS; i++) {
		PRINTF("%11s : %d\n", keys_raw[i], key_matches[i]);
	}

    pthread_attr_destroy(&attr);
    free(tids);
    free(targs);
}


static void parse_args(int argc, char** argv)
{
    int c;

    while((c = getopt(argc, argv, "f:n:t:h?")) != -1)
    {
        switch(c)
        {
        case 'f': fdata_name = optarg; break;
        case 'n': num_nodes = atoi(optarg); break;
        case 't': threads_per_node = atoi(optarg); break;
		case 'h':
        case '?':
            printf("Usage: -f <key file> -n <nodes> -t <threads per node>\n");
            exit(1);
            break;
        }
    }

    if(!fdata_name)
    {
        printf("Please specify a key file using -f\n");
        exit(1);
    }

    if(num_nodes < 1 || threads_per_node < 1)
    {
        printf("Invalid argument.  All integer values must be > 0\n");
        exit(1);
    }

    printf("Key file = %s\n", fdata_name);
    printf("Number of nodes = %d\n", num_nodes);
    printf("Threads per node = %d\n", threads_per_node);
}

int main(int argc, char *argv[])
{
    int fdata_fd;
    char *fdata;
    struct stat fdata_stat;
    struct timeval starttime, endtime;

    parse_args(argc, argv);

    // Read in the file
    fdata_fd = open(fdata_name, O_RDONLY);
	if (fdata_fd < 0) {
		fprintf(stderr, "Cannot open data file %s\n", fdata_name);
		return -ENOENT;
	}
    // Get the file info (for file length)
    fstat(fdata_fd, &fdata_stat);
#ifdef NO_MMAP
	fdata = (char *)malloc(fdata_stat.st_size);
	assert(!fdata);
    int ret = read(fdata_fd, fdata, fdata_stat.st_size);
    assert(ret == fdata_stat.st_size);
#else
    // Memory map the file
    fdata = mmap(0, fdata_stat.st_size + 1,
        PROT_READ, MAP_PRIVATE, fdata_fd, 0);
#endif

    PRINTF("File size is %lu\n", fdata_stat.st_size);
    PRINTF("grep: calling pthread grep\n");

    gettimeofday(&starttime,0);
    string_match(fdata, fdata_stat.st_size);
    gettimeofday(&endtime,0);

    printf("grep: Completed %.6lf\n\n", stopwatch_elapsed(&starttime, &endtime));

#ifdef NO_MMAP
    free(fdata);
#else
	munmap(fdata, fdata_stat.st_size + 1);
#endif
	close(fdata_fd);

    return 0;
}

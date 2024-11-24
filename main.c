#include <stdlib.h> /* NULL size_t etc */
#include <pthread.h> /* parallelism */
#include <math.h> /* min max sqrt */
#include <stdio.h> /* printf */
#include <unistd.h> /* sysconf */
#include <string.h>

/* Type to represent a Mapper thread */
typedef struct {
    int thread_id;
    int num_files;
    char **files;
} thread_data_t;

/* Mapper thread function */
void *thread_mapper(void *arg) {
    int thread_id = *((int *)arg);
}

/* Reducer thread function */
void *thread_reducer(void *arg) {
    int thread_id = *((int *)arg);
}

int main(int argc, char **argv)
{
    /* Initialise number of cores */
    long cores = sysconf(_SC_NPROCESSORS_CONF);
    printf("Number of cores: %ld.\n", cores);
    cores = 2;
    printf("Testing with: %ld.\n", cores);

    if (argc != 4) {
        printf("Format: ./tema1 <numar_mapperi> <numar_reduceri> <fisier_intrare>\n");
        exit(-1);
    }

    /* Read input file, distribute files */
    int num_mappers = atoi(argv[1]);
    int num_reducers = atoi(argv[2]);
    char *input_file = argv[3];
    return 0;
}
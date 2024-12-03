#ifndef MAIN_H
#define MAIN_H

#include <stdbool.h>
#include <stddef.h>
#include <pthread.h>
#include "helper.h"

/* Define input files type */
typedef struct {
    long thread_id;
    size_t *current_file;
    size_t *current_partial_list;

    char **files;
    size_t num_files;
    size_t num_mappers;
    size_t num_reducers;

    pthread_mutex_t *mutex_mapper;
    pthread_mutex_t *mutex_reducer;
    pthread_barrier_t *barrier;
    pthread_barrier_t *barrier_reducer;

    partial_list_vector_t *partial_lists;
    aggregate_list_vector_t *aggregate_lists;
} thread_arguments_t;

#endif /* MAIN_H */
#pragma once

#include "helper.h"
#include <pthread.h>
#include <sys/types.h>

/* Datatype that holds all the shared
    memory between threads */
typedef struct {
    long thread_id;
    /* Iterators for simulating a thread pool */
    size_t *current_file;
    size_t *current_partial_list;
    size_t *current_letter;

    /* Input files names and number */
    char **files;
    size_t num_files;

    /* Number of threads (argv) */
    size_t num_mappers;
    size_t num_reducers;

    /* Synchronization primitives */
    pthread_mutex_t *mutex_mapper;
    pthread_mutex_t *mutex_reducer;
    pthread_mutex_t *aggregate_mutex;
    pthread_barrier_t *barrier;
    pthread_barrier_t *barrier_reducer;

    /* Partial lists and Aggregate Lists
        (Map-Reduce Paradigm ) */
    partial_list_vector_t *partial_lists;
    aggregate_list_vector_t *aggregate_lists;
} thread_arguments_t;
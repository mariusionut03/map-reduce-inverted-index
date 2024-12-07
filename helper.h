#pragma once

#include <stdio.h>
#include <stdlib.h>
#include "string.h"
#include "stdbool.h"

typedef struct {
    char *word;
    size_t file_id;
} partial_list_t;

typedef struct {
    char *word;
    size_t *file_ids;
    size_t file_ids_count;
} aggregate_list_t;

typedef struct {
    partial_list_t *data;
    size_t size;
    size_t capacity;
} partial_list_vector_t;

typedef struct {
    aggregate_list_t *data;
    size_t size;
    size_t capacity;
} aggregate_list_vector_t;

/* Read input */
void read_input_file(char ***files, size_t *num_files, const char *input_file);
/* Transform words */
void transform_word(char *word);

/* Handle partial list */
void init_partial_list_vector(partial_list_vector_t *vector);
void add_partial_list(partial_list_vector_t *vector, const char *word, size_t file_id);
void destroy_partial_list_vector(partial_list_vector_t *vector);

/* Handle aggregate list */
void init_aggregate_list_vector(aggregate_list_vector_t *vector);
void add_aggregate_list(aggregate_list_vector_t *vector, const char *word, size_t file_id);
void destroy_aggregate_list_vector(aggregate_list_vector_t *vector);

/* Comparison function for sorting items by
    file ids count, then lexicographically */
int compare_aggregate_list(const void *a, const void *b);
/* Comparison function for sorting file IDs */
int compare_file_ids(const void *a, const void *b);
/* Function to sort file IDs within each aggregate list */
void sort_file_ids(aggregate_list_t *aggregate_list);
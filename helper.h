#ifndef HELPER_H
#define HELPER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
void clean_word(char *word);

/* Handle partial list */
void init_partial_list_vector(partial_list_vector_t *vector);
void add_partial_list(partial_list_vector_t *vector, const char *word, size_t file_id);
void destroy_partial_list_vector(partial_list_vector_t *vector);

/* Handle aggregate list */
void init_aggregate_list_vector(aggregate_list_vector_t *vector);
void add_aggregate_list(aggregate_list_vector_t *vector, const char *word, size_t file_id);
void destroy_aggregate_list_vector(aggregate_list_vector_t *vector);
int compare_aggregate_list(const void *a, const void *b);

/* Print functions */
void print_files(char **files, size_t num_files);
void print_partial_lists(partial_list_vector_t *partial_lists, size_t num_files);
void print_aggregate_lists(aggregate_list_vector_t *aggregate_lists);

#endif /* HELPER_H */
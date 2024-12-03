#include "helper.h"

#include <stdio.h>
#include <ctype.h>

/* Function to read the input file */
void read_input_file(char ***files, size_t *num_files, const char *input_file) {
    FILE *file = fopen(input_file, "r");
    if (file == NULL) {
        printf("Eroare la deschiderea fisierului primit ca argument(%s).\n", input_file);
        exit(-1);
    }

    fscanf(file, "%lu", num_files);
    *files = malloc(*num_files * sizeof(char *));
    for (size_t i = 0; i < *num_files; i++) {
        (*files)[i] = malloc(256 * sizeof(char));
        fscanf(file, "%s", (*files)[i]);
    }
    fclose(file);
}

void clean_word(char *word) {
    int i, j;
    for (i = 0, j = 0; word[i]; i++) {
        if (isalpha(word[i])) {
            word[j++] = tolower(word[i]);
        }
    }
    word[j] = '\0';
}

/* Handle partial lists functions */

void init_partial_list_vector(partial_list_vector_t *vector) {
    vector->data = NULL;
    vector->size = 0;
    vector->capacity = 0;
}

void add_partial_list(partial_list_vector_t *vector, const char *word, size_t file_id) {
    for (size_t i = 0; i < vector->size; i++) {
        if (strcmp(vector->data[i].word, word) == 0) {
            // Word already exists, do not add
            return;
        }
    }
    if (vector->size == vector->capacity) {
        vector->capacity = vector->capacity == 0 ? 1 : vector->capacity * 2;
        vector->data = realloc(vector->data, vector->capacity * sizeof(partial_list_t));
    }
    vector->data[vector->size].word = strdup(word);
    vector->data[vector->size].file_id = file_id;
    vector->size++;
}

void destroy_partial_list_vector(partial_list_vector_t *vector) {
    for (size_t i = 0; i < vector->size; i++) {
        free(vector->data[i].word);
    }
    free(vector->data);
    vector->data = NULL;
    vector->size = 0;
    vector->capacity = 0;
}

/* Handle aggregate lists functions */

void init_aggregate_list_vector(aggregate_list_vector_t *vector) {
    vector->data = NULL;
    vector->size = 0;
    vector->capacity = 0;
}

void add_aggregate_list(aggregate_list_vector_t *vector, const char *word, size_t file_id) {
    for (size_t i = 0; i < vector->size; i++) {
        if (strcmp(vector->data[i].word, word) == 0) {
            // Word already exists, add file_id to the existing item
            vector->data[i].file_ids = realloc(vector->data[i].file_ids, (vector->data[i].file_ids_count + 1) * sizeof(size_t));
            vector->data[i].file_ids[vector->data[i].file_ids_count] = file_id;
            vector->data[i].file_ids_count++;
            return;
        }
    }
    if (vector->size == vector->capacity) {
        vector->capacity = vector->capacity == 0 ? 1 : vector->capacity * 2;
        vector->data = realloc(vector->data, vector->capacity * sizeof(aggregate_list_t));
    }
    vector->data[vector->size].word = strdup(word);
    vector->data[vector->size].file_ids = malloc(sizeof(size_t));
    vector->data[vector->size].file_ids[0] = file_id;
    vector->data[vector->size].file_ids_count = 1;
    vector->size++;
}

void destroy_aggregate_list_vector(aggregate_list_vector_t *vector) {
    for (size_t i = 0; i < vector->size; i++) {
        free(vector->data[i].word);
        free(vector->data[i].file_ids);
    }
    free(vector->data);
    vector->data = NULL;
    vector->size = 0;
    vector->capacity = 0;
}

/* Comparison function for sorting aggregate lists */
int compare_aggregate_list(const void *a, const void *b) {
    aggregate_list_t *item1 = (aggregate_list_t *)a;
    aggregate_list_t *item2 = (aggregate_list_t *)b;

    // First compare by the number of file IDs
    if (item1->file_ids_count != item2->file_ids_count) {
        return item2->file_ids_count - item1->file_ids_count;
    }

    // If the number of file IDs is the same, compare lexicographically
    return strcmp(item1->word, item2->word);
}

/* Print functions */
void print_files(char **files, size_t num_files) {
    for (size_t i = 0; i < num_files; i++) {
        printf("%ld) %s\n", i, files[i]);
    }
}

void print_partial_lists(partial_list_vector_t *partial_lists, size_t num_files) {
    for (size_t i = 0; i < num_files; i++) {
        printf("Partial list %zu:\n", i);
        for (size_t j = 0; j < partial_lists[i].size; j++) {
            printf("  Word: %s, File ID: %zu\n", partial_lists[i].data[j].word, partial_lists[i].data[j].file_id);
        }
    }
}

void print_aggregate_lists(aggregate_list_vector_t *aggregate_lists) {
    for (size_t i = 0; i < 26; i++) {
        printf("Aggregate list %c:\n", (char)('a' + i));
        for (size_t j = 0; j < aggregate_lists[i].size; j++) {
            printf("  Word: %s, File IDs: ", aggregate_lists[i].data[j].word);
            for (size_t k = 0; k < aggregate_lists[i].data[j].file_ids_count; k++) {
                printf("%zu ", aggregate_lists[i].data[j].file_ids[k]);
            }
            printf("\n");
        }
    }
}
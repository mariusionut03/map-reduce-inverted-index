#include <stdlib.h> /* NULL size_t etc */
#include <pthread.h> /* parallelism */
#include <math.h> /* min max sqrt */
#include <stdio.h> /* printf */
#include <unistd.h> /* sysconf */
#include <string.h>
#include <stdbool.h>
#include <ctype.h> /* isalpha */
#include <bits/pthreadtypes.h>

/* Synchronization primitives */
pthread_mutex_t mapper_mutex;
pthread_mutex_t reducer_mutex;
pthread_barrier_t barrier;

/* Define input files type */
typedef struct {
    char *file_name;
    bool isProcessed;
} file_data_t;

typedef struct {
    char *word;
    size_t file_id;
} partial_list_t;

typedef struct {
    char *word;
    size_t *file_ids;
    size_t file_ids_count;
} aggregate_list_t;

size_t files_size;
size_t current_file;
file_data_t *files;
partial_list_t **partial_list;
size_t *partial_list_size;
aggregate_list_t *aggregate_lists[26];
size_t aggregate_lists_size[26];
size_t current_p_list;

/* Read input file */
void read_input_file(char *input_file) {
    FILE *file = fopen(input_file, "r");
    if (file == NULL) {
        printf("Eroare la deschiderea fisierului de intrare(%s).\n", input_file);
        exit(-1);
    }

    fscanf(file, "%ld", &files_size);
    files = calloc(files_size, sizeof(file_data_t));
    for (size_t i = 0; i < files_size; i++) {
        files[i].file_name = malloc(256 * sizeof(char));
        files[i].isProcessed = 0;
        fscanf(file, "%s", files[i].file_name);
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

/* Mapper thread function */
void *thread_mapper(void *arg) {
    // long thread_id = *((long *)arg);
    long assigned_file_index;

    while (true) {

        pthread_mutex_lock(&mapper_mutex);
        if (current_file >= files_size) {
            pthread_mutex_unlock(&mapper_mutex);
            break;
        }

        if (files[current_file].isProcessed == 0) {
            assigned_file_index = current_file;
            files[current_file++].isProcessed = 1;
        } else {
            pthread_mutex_unlock(&mapper_mutex);
            continue;
        }
        
        pthread_mutex_unlock(&mapper_mutex);

        FILE *assigned_file = fopen(files[assigned_file_index].file_name, "r");
        if (assigned_file == NULL) {
            printf("Error opening file: %s\n", files[assigned_file_index].file_name);
            exit(-1);
        }

        char *buffer = NULL;
        size_t bufsize = 0;
        ssize_t line_length;

        while ((line_length = getline(&buffer, &bufsize, assigned_file)) != -1) {
            char *word = strtok(buffer, " .,?!\t\n");
            while (word != NULL) {
                clean_word(word);
                if (strlen(word) > 0) {
                    if (partial_list_size[assigned_file_index] == 0) {
                        partial_list[assigned_file_index] = calloc(1, sizeof(partial_list_t));
                    } else {
                        partial_list[assigned_file_index] = realloc(partial_list[assigned_file_index], partial_list_size[assigned_file_index] * sizeof(partial_list_t) + sizeof(partial_list_t));
                    }

                    partial_list[assigned_file_index][partial_list_size[assigned_file_index]].word = malloc(strlen(word) + 1);
                    strcpy(partial_list[assigned_file_index][partial_list_size[assigned_file_index]].word, word);
                    partial_list[assigned_file_index][partial_list_size[assigned_file_index]].file_id = assigned_file_index;
                    partial_list_size[assigned_file_index] += 1;
                }
                word = strtok(NULL, " .,?!\t\n");
            }
        }
        free(buffer);
        buffer = NULL;

        fclose(assigned_file);
    }

    pthread_barrier_wait(&barrier);
    return NULL;
}

/* Reducer thread function */
void *thread_reducer(void *arg) {
    pthread_barrier_wait(&barrier);
    // long thread_id = *((long *)arg);
    size_t current_idx;
    while (true) {
        pthread_mutex_lock(&reducer_mutex);
        if (current_p_list >= files_size) {
            pthread_mutex_unlock(&reducer_mutex);
            break;
        } else {
            current_idx = current_p_list;
            current_p_list++;
        }
        pthread_mutex_unlock(&reducer_mutex);

        for (size_t i = 0; i < partial_list_size[current_idx]; i++) {
            char *word = partial_list[current_idx][i].word;
            if (aggregate_lists[word[0] - 'a'] == NULL) {
                /* List is empty*/
                aggregate_lists[word[0] - 'a'] = calloc(1, sizeof(aggregate_list_t));
                aggregate_lists[word[0] - 'a'][0].word = malloc(strlen(word) + 1);
                strcpy(aggregate_lists[word[0] - 'a'][0].word, word);
                aggregate_lists[word[0] - 'a'][0].file_ids = calloc(1, sizeof(size_t));
                aggregate_lists[word[0] - 'a'][0].file_ids[0] = partial_list[current_idx][i].file_id;
                aggregate_lists[word[0] - 'a'][0].file_ids_count = 1;
                aggregate_lists_size[word[0] - 'a'] += 1;
            } else {
                /* Add element / fileid */
                bool found = false;
                for (size_t j = 0; j < aggregate_lists_size[word[0] - 'a']; j++) {
                    if (strcmp(word, aggregate_lists[word[0] - 'a'][j].word) == 0) {
                        /* Found word */
                        bool file_id_exists = false;
                        for (size_t k = 0; k < aggregate_lists[word[0] - 'a'][j].file_ids_count; k++) {
                            if (aggregate_lists[word[0] - 'a'][j].file_ids[k] == partial_list[current_idx][i].file_id) {
                                file_id_exists = true;
                                break;
                            }
                        }
                        if (!file_id_exists) {
                            aggregate_lists[word[0] - 'a'][j].file_ids = realloc(aggregate_lists[word[0] - 'a'][j].file_ids, (aggregate_lists[word[0] - 'a'][j].file_ids_count + 1) * sizeof(size_t));
                            aggregate_lists[word[0] - 'a'][j].file_ids[aggregate_lists[word[0] - 'a'][j].file_ids_count] = partial_list[current_idx][i].file_id;
                            aggregate_lists[word[0] - 'a'][j].file_ids_count += 1;
                        }
                        found = true;
                        break;
                    }
                }
                if (found == false) {
                    aggregate_lists_size[word[0] - 'a'] += 1;
                    aggregate_lists[word[0] - 'a'] = realloc(aggregate_lists[word[0] - 'a'], aggregate_lists_size[word[0] - 'a'] * sizeof(aggregate_list_t));
                    aggregate_lists[word[0] - 'a'][aggregate_lists_size[word[0] - 'a'] - 1].word = malloc(strlen(word) + 1);
                    strcpy(aggregate_lists[word[0] - 'a'][aggregate_lists_size[word[0] - 'a'] - 1].word, word);
                    aggregate_lists[word[0] - 'a'][aggregate_lists_size[word[0] - 'a'] - 1].file_ids = malloc(sizeof(size_t));
                    aggregate_lists[word[0] - 'a'][aggregate_lists_size[word[0] - 'a'] - 1].file_ids[0] = partial_list[current_idx][i].file_id;
                    aggregate_lists[word[0] - 'a'][aggregate_lists_size[word[0] - 'a'] - 1].file_ids_count = 1;
                }
            }
        }
    }
    return NULL;
}

int main(int argc, char **argv) {
    /* Initialise number of threads */
    long cores = sysconf(_SC_NPROCESSORS_CONF);
    printf("Available cores on this device: %ld.\n", cores);

    if (argc != 4) {
        printf("Format: ./tema1 <numar_mapperi> <numar_reduceri> <fisier_intrare>\n");
        exit(-1);
    }

    
    long num_mappers = atoi(argv[1]);
    long num_reducers = atoi(argv[2]);
    char *input_file = argv[3];
    long long number_of_threads = num_mappers + num_reducers;


    read_input_file(input_file);

    /* Create threads */
    pthread_t threads[number_of_threads];
    int r;
    long id;
    void *status;
    long arguments[number_of_threads];

    partial_list = calloc(files_size, sizeof(partial_list_t *));
    partial_list_size = calloc(files_size, sizeof(long));

    pthread_mutex_init(&mapper_mutex, NULL);
    pthread_barrier_init(&barrier, NULL, number_of_threads);
    pthread_mutex_init(&reducer_mutex, NULL);
    current_file = 0;
    current_p_list = 0;
    for (int i = 0; i < 26; i++) {
        aggregate_lists_size[i] = 0;
    }

    for (id = 0; id < number_of_threads; id++) {
        arguments[id] = id;
        r = pthread_create(&threads[id], NULL, id < num_mappers ? thread_mapper : thread_reducer, (void *) &arguments[id]);
        if (r) {
            printf("Eroare la crearea thread-ului %ld\n", id);
            exit(-1);
        }
    }

    /* Wait for all the threads */
    for (id = 0; id < number_of_threads; id++) {
        r = pthread_join(threads[id], &status);
        if (r) {
            printf("Eroare la asteptarea thread-ului %ld\n", id);
            exit(-1);
        }
    }

    // Print partial lists
    for (size_t i = 0; i < files_size; i++) {
        printf("partialList[%ld]: ", i);
        for (size_t j = 0; j < partial_list_size[i]; j++) {
            printf("%s[%ld]; ", partial_list[i][j].word, partial_list[i][j].file_id);
        }
        printf("\n");
    }

    // Print aggregated lists
    for (int i = 0; i < 26; i++) {
        if (aggregate_lists[i] != NULL) {
            printf("%c: ", 'a' + i);
            for (size_t j = 0; j < aggregate_lists_size[i]; j++) {
                printf("%s [", aggregate_lists[i][j].word);
                for (size_t k = 0; k < aggregate_lists[i][j].file_ids_count; k++) {
                    printf("%ld", aggregate_lists[i][j].file_ids[k]);
                    if (k < aggregate_lists[i][j].file_ids_count - 1) {
                        printf(" ");
                    }
                }
                printf("]; ");
            }
            printf("\n");
        }
    }

    // Free allocated memory
    for (size_t i = 0; i < files_size; i++) {
        free(files[i].file_name);
    }

    for (size_t i = 0; i < files_size; i++) {
        for (size_t j = 0; j < partial_list_size[i]; j++) {
            free(partial_list[i][j].word);
        }
        free(partial_list[i]);
    }

    for (int i = 0; i < 26; i++) {
        for (size_t j = 0; j < aggregate_lists_size[i]; j++) {
            free(aggregate_lists[i][j].file_ids);
            free(aggregate_lists[i][j].word);
        }
        free(aggregate_lists[i]);
    }

    free(files);
    free(partial_list);
    free(partial_list_size);

    // Destroy the mutex
    pthread_mutex_destroy(&mapper_mutex);
    pthread_barrier_destroy(&barrier);
    pthread_mutex_destroy(&reducer_mutex);

    return 0;
}
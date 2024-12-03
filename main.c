#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stddef.h>
#include "helper.h"
#include "main.h"

/* Function for Mapper Thread */
void *thread_mapper(void *arg) {
    thread_arguments_t arguments = *(thread_arguments_t *)arg;

    size_t fid;

    while (true) {
        pthread_mutex_lock(arguments.mutex_mapper);

        if (*(arguments.current_file) >= arguments.num_files) {
            pthread_mutex_unlock(arguments.mutex_mapper);
            break;
        }

        fid = *(arguments.current_file);
        *(arguments.current_file) += 1;

        pthread_mutex_unlock(arguments.mutex_mapper);

        FILE *file = fopen(arguments.files[fid], "r");
        if (file == NULL) {
            printf("Eroare la deschiderea fisierului: %s\n", arguments.files[fid]);
            exit(-1);
        }

        char *buffer = NULL;
        size_t bufsize = 0;
        ssize_t line_length;

        while ((line_length = getline(&buffer, &bufsize, file)) != -1) {
            char *saveptr;
            char *word = strtok_r(buffer, " “”.,?!\t\n", &saveptr);
            while (word != NULL) {
                clean_word(word);
                if (strlen(word) > 0) {
                    add_partial_list(&(arguments.partial_lists[fid]), word, fid + 1);
                }
                word = strtok_r(NULL, " “”.,?!\t\n", &saveptr);
            }
        }
        free(buffer);
        buffer = NULL;
        fclose(file);
    }

    pthread_barrier_wait(arguments.barrier);
    return NULL;
}

/* Function for Reducer Thread */
void *thread_reducer(void *arg) {
    thread_arguments_t arguments = *(thread_arguments_t *)arg;
    pthread_barrier_wait(arguments.barrier);

    size_t lid;
    while (true) {
        pthread_mutex_lock(arguments.mutex_reducer);
        if (*(arguments.current_partial_list) >= arguments.num_files) {
            pthread_mutex_unlock(arguments.mutex_reducer);
            break;
        }
        lid = *(arguments.current_partial_list);
        *(arguments.current_partial_list) += 1;
        pthread_mutex_unlock(arguments.mutex_reducer);

        for (size_t i = 0; i < arguments.partial_lists[lid].size; i++) {
            char *word = arguments.partial_lists[lid].data[i].word;
            pthread_mutex_lock(arguments.mutex_reducer);
            add_aggregate_list(&(arguments.aggregate_lists[word[0] - 'a']),
                                word,
                                arguments.partial_lists[lid].data[i].file_id);
            pthread_mutex_unlock(arguments.mutex_reducer);
        }
    }

    pthread_barrier_wait(arguments.barrier_reducer);
    // Divide the sorting work among the threads
    size_t num_threads = arguments.num_reducers;
    size_t thread_id = arguments.thread_id - arguments.num_mappers; // Adjust thread_id for reducer threads
    size_t lists_per_thread = 26 / num_threads;
    size_t start_index = thread_id * lists_per_thread;
    size_t end_index = (thread_id == num_threads - 1) ? 26 : start_index + lists_per_thread;

    for (size_t i = start_index; i < end_index; i++) {
        pthread_mutex_lock(arguments.mutex_reducer);
        qsort(arguments.aggregate_lists[i].data, arguments.aggregate_lists[i].size, sizeof(aggregate_list_t), compare_aggregate_list);
        pthread_mutex_unlock(arguments.mutex_reducer);
    }

    pthread_barrier_wait(arguments.barrier_reducer);

    // Write the output files
    for (size_t i = start_index; i < end_index; i++) {
        char filename[6];
        pthread_mutex_lock(arguments.mutex_reducer);
        snprintf(filename, sizeof(filename), "%c.txt", (char)('a' + i));
        FILE *output_file = fopen(filename, "w");
        if (output_file == NULL) {
            printf("Eroare la deschiderea fisierului de iesire: %s\n", filename);
            exit(-1);
        }

        for (size_t j = 0; j < arguments.aggregate_lists[i].size; j++) {
            fprintf(output_file, "%s:[", arguments.aggregate_lists[i].data[j].word);
            for (size_t k = 0; k < arguments.aggregate_lists[i].data[j].file_ids_count; k++) {
                fprintf(output_file, "%zu", arguments.aggregate_lists[i].data[j].file_ids[k]);
                if (k < arguments.aggregate_lists[i].data[j].file_ids_count - 1) {
                    fprintf(output_file, " ");
                }
            }
            fprintf(output_file, "]\n");
        }

        fclose(output_file);
        pthread_mutex_unlock(arguments.mutex_reducer);
    }

    return NULL;
}

int main(int argc, char **argv) {
    /* Check and store command line arguments */
    if (argc != 4) {
        printf("Format: ./tema1 <numar_mapperi> <numar_reduceri> <fisier_intrare>\n");
        exit(-1);
    }

    long num_mappers = atoi(argv[1]);
    long num_reducers = atoi(argv[2]);
    char *input_file = argv[3];

    /* Read and store files */
    char **files = NULL;
    size_t num_files;
    size_t current_file = 0;
    size_t current_partial_list = 0;

    read_input_file(&files, &num_files, input_file);
    print_files(files, num_files);

    /* Initialize partial and aggregate lists */
    partial_list_vector_t *partial_lists = malloc(num_files * sizeof(partial_list_vector_t));
    for (size_t i = 0; i < num_files; i++) {
        init_partial_list_vector(&partial_lists[i]);
    }

    aggregate_list_vector_t *aggregate_lists = malloc(26 * sizeof(aggregate_list_vector_t));
    for (size_t i = 0; i < 26; i++) {
        init_aggregate_list_vector(&aggregate_lists[i]);
    }

    /* Define thread creation and join arguments */
    pthread_t threads[num_mappers + num_reducers];
    int r;
    long id;
    void *status;
    thread_arguments_t arguments[num_mappers + num_reducers];

    /* Define and initialise synchronisation primitives */
    pthread_mutex_t mutex_mapper;
    pthread_mutex_t mutex_reducer;
    pthread_barrier_t barrier;
    pthread_barrier_t barrier_reducer;
    r = pthread_mutex_init(&mutex_mapper, NULL);
    r = pthread_mutex_init(&mutex_reducer, NULL);
    r = pthread_barrier_init(&barrier, NULL, num_mappers + num_reducers);
    r = pthread_barrier_init(&barrier_reducer, NULL, num_reducers);

    for (id = 0; id < num_mappers + num_reducers; id++) {
        arguments[id].thread_id = id;
        arguments[id].current_file = &current_file;
        arguments[id].current_partial_list = &current_partial_list;
        arguments[id].files = files;
        arguments[id].num_files = num_files;
        arguments[id].num_mappers = num_mappers;
        arguments[id].num_reducers = num_reducers;

        arguments[id].mutex_mapper = &mutex_mapper;
        arguments[id].mutex_reducer = &mutex_reducer;
        arguments[id].barrier = &barrier;
        arguments[id].barrier_reducer = &barrier_reducer;
        
        arguments[id].partial_lists = partial_lists;
        arguments[id].aggregate_lists = aggregate_lists;

        r = pthread_create(&threads[id], NULL, id < num_mappers ? thread_mapper : thread_reducer, (void *) &arguments[id]);
        if (r) {
            printf("Eroare la crearea thread-ului %ld.\n", id);
            exit(-1);
        }
    }

    for (id = 0; id < num_mappers + num_reducers; id++) {
        r = pthread_join(threads[id], &status);
        if (r) {
            printf("Eroare la asteptarea thread-ului %ld.\n", id);
            exit(-1);
        }
    }

    /* Print partial lists */
    // print_partial_lists(partial_lists, num_files);

    /* Print aggregate lists */
    // print_aggregate_lists(aggregate_lists);

    /* Free allocated memory */
    for (size_t i = 0; i < num_files; i++) {
        destroy_partial_list_vector(&partial_lists[i]);
    }
    free(partial_lists);

    for (size_t i = 0; i < 26; i++) {
        destroy_aggregate_list_vector(&aggregate_lists[i]);
    }
    free(aggregate_lists);

    for (size_t i = 0; i < num_files; i++) {
        free(files[i]);
    }
    free(files);

    return 0;
}
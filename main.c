#include "helper.h"
#include "main.h"
#include <sys/stat.h>
#include <sys/types.h>

/* Function launched by a Mapper Thread */
void *thread_mapper(void *arg) {
    thread_arguments_t arguments = *(thread_arguments_t *)arg;

    size_t fid;

    while (true) {
        /* Take an unprocessed file */
        pthread_mutex_lock(arguments.mutex_mapper);
        if (*(arguments.current_file) >= arguments.num_files) {
            pthread_mutex_unlock(arguments.mutex_mapper);
            break;
        }
        fid = *(arguments.current_file);
        *(arguments.current_file) += 1;
        pthread_mutex_unlock(arguments.mutex_mapper);

        /* Open associated file */
        char filepath[256];
        snprintf(filepath, sizeof(filepath), "tests/%s", arguments.files[fid]);
        FILE *file = fopen(filepath, "r");
        if (file == NULL) {
            printf("Eroare la deschiderea fisierului: %s\n", filepath);
            exit(-1);
        }

        char *buffer = NULL;
        size_t bufsize = 0;
        ssize_t line_length;

        /* Read line by line */
        while ((line_length = getline(&buffer, &bufsize, file)) != -1) {
            char *saveptr;
            /* Use thread_safe strtok_r to divide the char array,
                clean the word and add it to the partial list */
            char *word = strtok_r(buffer, " \t\n", &saveptr);
            while (word != NULL) {
                transform_word(word);
                if (strlen(word) > 0) {
                    add_partial_list(&(arguments.partial_lists[fid]), word, fid + 1);
                }
                word = strtok_r(NULL, " \t\n", &saveptr);
            }
        }
        free(buffer);
        buffer = NULL;
        fclose(file);
    }

    /* Wait for all the threads before moving on. Used to
        make sure reducer threads start after mapper ones. */
    pthread_barrier_wait(arguments.barrier);
    return NULL;
}

/* Function launched by a Reducer Thread */
void *thread_reducer(void *arg) {
    thread_arguments_t arguments = *(thread_arguments_t *)arg;

    /* Wait before all the mapper threads finish their work */
    pthread_barrier_wait(arguments.barrier);

    size_t lid;
    while (true) {
        /* Take an unprocessed parial list */
        pthread_mutex_lock(arguments.mutex_reducer);
        if (*(arguments.current_partial_list) >= arguments.num_files) {
            pthread_mutex_unlock(arguments.mutex_reducer);
            break;
        }
        lid = *(arguments.current_partial_list);
        *(arguments.current_partial_list) += 1;
        pthread_mutex_unlock(arguments.mutex_reducer);

        /* Place words in corresponding aggregate list
            (one for every letter of the alphabet) */
        for (size_t i = 0; i < arguments.partial_lists[lid].size; i++) {
            char *word = arguments.partial_lists[lid].data[i].word;
            pthread_mutex_lock(&arguments.aggregate_mutex[word[0] - 'a']);
            add_aggregate_list(&(arguments.aggregate_lists[word[0] - 'a']),
                                word,
                                arguments.partial_lists[lid].data[i].file_id);
            pthread_mutex_unlock(&arguments.aggregate_mutex[word[0] - 'a']);
        }
    }

    /* Wait untill all the aggregate lists are built */
    pthread_barrier_wait(arguments.barrier_reducer);

    size_t fid;
    while (true) {
        /* Take an unprocessed aggregate list */
        pthread_mutex_lock(arguments.mutex_reducer);
        if (*(arguments.current_letter) >= 26) {
            pthread_mutex_unlock(arguments.mutex_reducer);
            break;
        }
        fid = *(arguments.current_letter);
        *(arguments.current_letter) += 1;
        pthread_mutex_unlock(arguments.mutex_reducer);

        /* Sort file ids for every word */
        for (size_t j = 0; j < arguments.aggregate_lists[fid].size; j++) {
            sort_file_ids(&(arguments.aggregate_lists[fid].data[j]));
        }

        /* Sort words */
        qsort(arguments.aggregate_lists[fid].data, arguments.aggregate_lists[fid].size, sizeof(aggregate_list_t), compare_aggregate_list);

        /* Open file and write output */
        char filename[19];
        snprintf(filename, sizeof(filename), "tests/output/%c.txt", (char)('a' + fid));
        
        /* Ensure the output directory exists */
        struct stat st = {0};
        if (stat("tests/output", &st) == -1) {
            mkdir("tests/output", 0700);
        }
        
        FILE *output_file = fopen(filename, "w");
        if (output_file == NULL) {
            printf("Eroare la deschiderea fisierului de iesire: %s\n", filename);
            exit(-1);
        }
        for (size_t j = 0; j < arguments.aggregate_lists[fid].size; j++) {
            fprintf(output_file, "%s:[", arguments.aggregate_lists[fid].data[j].word);
            for (size_t k = 0; k < arguments.aggregate_lists[fid].data[j].file_ids_count; k++) {
                fprintf(output_file, "%zu", arguments.aggregate_lists[fid].data[j].file_ids[k]);
                if (k < arguments.aggregate_lists[fid].data[j].file_ids_count - 1) {
                    fprintf(output_file, " ");
                }
            }
            fprintf(output_file, "]\n");
        }

        fclose(output_file);
    }

    return NULL;
}

int main(int argc, char **argv) {
    /* Check and store command line arguments */
    if (argc != 4) {
        printf("Format: ./main <numar_mapperi> <numar_reduceri> <fisier_intrare>\n");
        exit(-1);
    }
    long num_mappers = atoi(argv[1]);
    long num_reducers = atoi(argv[2]);
    char *input_file = argv[3];

    /* Read and store files */
    char **files = NULL;
    size_t num_files;

    read_input_file(&files, &num_files, input_file);

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

    size_t current_file = 0;
    size_t current_partial_list = 0;
    size_t current_letter = 0;

    /* Define and initialise synchronisation primitives */
    pthread_mutex_t mutex_mapper;
    pthread_mutex_t mutex_reducer;
    pthread_mutex_t aggregate_mutex[26];

    pthread_barrier_t barrier;
    pthread_barrier_t barrier_reducer;

    r = pthread_mutex_init(&mutex_mapper, NULL);
    r = pthread_mutex_init(&mutex_reducer, NULL);
    for (int i = 0; i < 26; ++i) {
        pthread_mutex_init(aggregate_mutex + i, NULL);
    }

    r = pthread_barrier_init(&barrier, NULL, num_mappers + num_reducers);
    r = pthread_barrier_init(&barrier_reducer, NULL, num_reducers);

    /* Build the argument and create the thread */
    for (id = 0; id < num_mappers + num_reducers; id++) {
        arguments[id].thread_id = id;
        arguments[id].current_file = &current_file;
        arguments[id].current_partial_list = &current_partial_list;
        arguments[id].current_letter = &current_letter;

        arguments[id].files = files;
        arguments[id].num_files = num_files;
        arguments[id].num_mappers = num_mappers;
        arguments[id].num_reducers = num_reducers;

        arguments[id].mutex_mapper = &mutex_mapper;
        arguments[id].mutex_reducer = &mutex_reducer;
        arguments[id].aggregate_mutex = aggregate_mutex;
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

    /* Wait for all the threads */
    for (id = 0; id < num_mappers + num_reducers; id++) {
        r = pthread_join(threads[id], &status);
        if (r) {
            printf("Eroare la asteptarea thread-ului %ld.\n", id);
            exit(-1);
        }
    }

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
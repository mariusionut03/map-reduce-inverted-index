# Map-Reduce Inverted Index

## Overview

This project implements a Map-Reduce Inverted Index using multithreading in C. The program reads multiple text files, processes the words, and creates an inverted index that maps each word to the list of files in which it appears.

## Components
- `helper.c`: Contains helper functions for handling partial and aggregate lists, reading input files, and transforming words.
- `helper.h`: Header file for `helper.c`, defines data structures and function prototypes.
- `main.c`: Contains the main logic of the program, including the implementation of mapper and reducer threads.
- `main.h`: Header file for `main.c`, defines the `thread_arguments_t` structure used for passing arguments to threads.
- `Makefile`: Build script for compiling the project.

## Features

### Mapper Threads

- **File Distribution**: The threads dynamically take a file from the queue when they finish processing a file, ensuring a good distribution in cases where files vary in size and length.
- **Partial Lists**: Mapper threads create a partial list for every file. These lists, after processing the entire file, will contain all the words (only once).

### Reducer Threads

- **Aggregate Lists**: 26 aggregate lists are created, one for each letter of the alphabet. Partial lists are processed, and words from them are added to these lists with the file ID as well.
- **Partial List Distribution**: Reducer threads dynamically take a partial list from the queue, ensuring a good distribution in cases where lists vary in size and length.
- **Sorting**: After all the words are added, the threads will sort the words in decreasing order by the number of files they were found in, then lexicographically. Immediately after sorting, the aggregate lists are written to the output files.

### Other Features

- **Good Memory Handling**: Valgrind tests declare 0 bytes in use at exit.
- **Synchronization**: Two barriers and 28 mutexes from the PThread C library are used to ensure the best thread synchronization possible while maintaining a fast runtime.

## How to Run
1. **Build the Project**: Use the following command:
    ```sh
    make build
    ```
    or
    ```sh
    gcc main.c helper.c -o main -lpthread -lm
    ```
2. **Add Your Input**: Add the files you want to process somewhere in the project. Recommended: Add your files to the `tests/` directory. Create a file with the following format:

    ```sh
    number_of_files
    path_to_file_1
    path_to_file_2
    ...
    path_to_file_N
    ```
3. **Run the Project**: Use the following command:
    ```sh
    ./main <num_mappers> <num_reducers> <input_file>
    ```

    - `<num_mappers>`: Number of mapper threads.
    - `<num_reducers>`: Number of reducer threads.
    - `<input_file>`: Path to the file containing the list of input files.

4. **Check the Output**: You can find the output in `tests/output`. There is a file for every letter of the alphabet. You can find the words starting with `X` in `X.txt`.

5. **Clean the Project**: To remove the executable and output files, use:

    ```sh
    make clean
    ```

## Example

To test using the smaller test provided, use:

    make build
    ./main 4 4 tests/test_small.txt

To test using the larger test provided, use:

    make build
    ./main 4 4 tests/test.txt

## Acknowledgements

The tests for this project were created by UPB's Moby Labs Team.

## License

This project is licensed under the MIT License.
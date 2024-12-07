# Map-Reduce Inverted Index

This project implements a Map-Reduce Inverted Index using multithreading in C. The program reads multiple text files, processes the words, and creates an inverted index that maps each word to the list of files in which it appears.

## Project Structure

- `helper.c`: Contains helper functions for handling partial and aggregate lists, reading input files, and transforming words.
- `helper.h`: Header file for `helper.c`, defines data structures and function prototypes.
- `main.c`: Contains the main logic of the program, including the implementation of mapper and reducer threads.
- `main.h`: Header file for `main.c`, defines the `thread_arguments_t` structure used for passing arguments to threads.
- `Makefile`: Build script for compiling the project.

## Build and Run

To build the project, use the following command:

```sh
make build
```

To clean the build artifacts, use:

```sh
make clean
```

To run the program, use the following format:

```sh
./tema1 <num_mappers> <num_reducers> <input_file>
```

- `<num_mappers>`: Number of mapper threads.
- `<num_reducers>`: Number of reducer threads.
- `<input_file>`: Path to the input file containing the list of text files to be processed.

## Input File Format
The input file should contain the number of text files followed by the paths to each text file, one per line. Example:

```text
3
files/file1.txt
files/file2.txt
files/file3.txt
```

## Output
The program generates 26 output files, one for each letter of the alphabet (a.txt, b.txt, ..., z.txt). Each file contains the words starting with the corresponding letter and the list of file IDs where each word appears.

## How is it working

I created a new datatype called `thread_arguments_t` to pass the shared memory as argument to the threads. Here we have variables to simulate multiple thread pools, synchronization primitives like mutexes and barriers, multiple lists explained further.

### Mapper thread
Using a thread pool, each mapper thread takes a file and builds it's partial list (a list that contains all the words found, only once, and the id of the current file).

### Reducer thread
Using a thread pool, each partial list is assosiated to a reducer and each word is redirected to a aggregate list. I created 26 lists, each one representing a letter (Got the idea from Moodle forum).

After all the work is done, we make another thread pool and take every aggregate list, sort it by the number of file ids, then lexicographically. In the end, we write the output to the files.
build:
	gcc main.c helper.c -o main -lpthread -lm -Wall -Wextra
clean:
	rm -r tests/output
	rm main
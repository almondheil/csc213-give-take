#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.h"

void take_file(char * give_user) {
	// Find the fifo this transaction will occur through
	char * fifo_name = find_fifo_name(give_user, getenv("LOGNAME"));
	int fifo_fd = open(fifo_name, O_RDONLY);
	if (fifo_fd == -1) {
		perror("Failed to open fifo");
		exit(EXIT_FAILURE);
	}

	// Read the size of the filename
	size_t filename_size;
	if (read(fifo_fd, &filename_size, sizeof(size_t)) != sizeof(size_t)) {
		perror("Failed to read filename size");
		exit(EXIT_FAILURE);
	}

	// Read the size of the file
	size_t file_size;
	if (read(fifo_fd, &file_size, sizeof(size_t)) != sizeof(size_t)) {
		perror("Failed to read file size");
		exit(EXIT_FAILURE);
	}

	printf("File size was %zu, name length was %zu\n", file_size, filename_size);
}

int main(int argc, char ** argv) {
	// TODO: Later, I may want a case for just running with argc == 1 -- list out pending files
	if (argc != 2) {
		printf("Usage: %s USER\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	// If the user trying to take from themselves, don't let them
	char * take_username = getenv("LOGNAME");
	if (take_username == NULL) {
		fprintf(stderr, "Error: Could not determine your username");
		exit(EXIT_FAILURE);
	}
	// TODO: Later, don't allow taking from yourself
	// if (strcmp(take_username, argv[1]) == 0) {
	// 	fprintf(stderr, "Error: Cannot take a file from yourself\n");
	// 	exit(EXIT_FAILURE);
	// }

	// Check that the user they are trying to take from exists
	struct passwd * from_user = getpwnam(argv[1]);
	if (from_user == NULL) {
		fprintf(stderr, "Error: Could not find the user %s\n", argv[1]);
		exit(EXIT_FAILURE);
	}

	take_file(argv[1]);

	return 0;
}

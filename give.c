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

// Send a file to a user. Returns when the sending is complete
// TODO: Document
int give_file(char * target_user, char * filename, FILE* file) {
	// Seek to the end of the file so we can get its size
	if (fseek(file, 0, SEEK_END) != 0) {
		perror("Could nit seek to end of file");
		return -1;
	}

	// Find the size
	size_t file_size = ftell(file);

	// Seek back to the start so we can read it
	if (fseek(file, 0, SEEK_SET) != 0) {
		perror("Could not seek to start of file");
		return -1;
	}

	// If the file is too big, refuse to send it
	if (file_size > MAX_FILE_SIZE) {
		fprintf(stderr, "File size is too large!\n");
		return -1;
	}

	// Allocate space, and make sure everything fits
	uint8_t *file_data = malloc(file_size);
	if (file_data == NULL) {
		perror("Could not allocate space to store file contents");
		return -1;
	}

	// Read the file data into the malloced space
	if (fread(file_data, 1, file_size, file) != file_size) {
		fprintf(stderr, "Failed to read the entire file\n");
		free(file_data);
		return -1;
	}

	// Determine a name for this fifo
	char * fifo_name = find_fifo_name(getenv("LOGNAME"), target_user);

	// Open that FIFO and check for any errors
	int rc = mknod(fifo_name, S_IFIFO | 0644, 0);
	if (rc == -1) {
		perror("Give failed to make FIFO");
		free(file_data);
		return -1;
	}

	// Open the FIFO. This call returns once we get a reader
	int fifo_fd = open(fifo_name, O_WRONLY);
	if (fifo_fd == -1) {
		perror("Give failed to open FIFO");
		free(file_data);
		remove(fifo_name);
		return -1;
	}

	// TODO: We do not check if the reader is the correct user.
	// For now we're just pretending it's okay

	// TODO: Probably want a message.c and .h with read and write stuff for this. It is ugh and hard.

	// Write the size of the filename through the fifo
	size_t filename_len = strlen(filename);
	if (write(fifo_fd, &filename_len, sizeof(size_t)) != sizeof(size_t)) {
		perror("Failed to write file length");
		free(file_data);
		remove(fifo_name);
		return -1;
	}

	// Write the size of the file through the fifo
	if (write(fifo_fd, &file_size, sizeof(size_t)) != sizeof(size_t)) {
		perror("Failed to write file length");
		free(file_data);
		remove(fifo_name);
		return -1;
	}

	// Send the name of the file
	size_t bytes_written = 0;
	while (bytes_written < filename_len) {
		// Write the remaining message
		ssize_t rc = write(fifo_fd, filename + bytes_written, filename_len - bytes_written);

		// If the write failed, there was an error
		if (rc <= 0) return -1;

		bytes_written += rc;
	}

	// Write the file contents through the fifo
	// TODO

	// Remove the FIFO to clean up after the communication
	free(file_data);
	remove(fifo_name);
	return 0;
}

// Entry point to the program.
int main(int argc, char ** argv) {
	// Check the right number of arguments have been supplied
	if (argc != 3) {
		fprintf(stderr, "Usage: %s USER FILE\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	// If the user trying to send to themselves, don't let them
	char * give_username = getenv("LOGNAME");
	if (give_username == NULL) {
		fprintf(stderr, "Error: Could not determine your username");
		exit(EXIT_FAILURE);
	}
	// TODO: Later, disable giving to yourself
	// if (strcmp(give_username, argv[1]) == 0) {
	// 	fprintf(stderr, "Error: Cannot give a file to yourself\n");
	// 	exit(EXIT_FAILURE);
	// }

	// Check that the user they are trying to send to exists
	struct passwd * to_user = getpwnam(argv[1]);
	if (to_user == NULL) {
		fprintf(stderr, "Error: Could not find the user %s\n", argv[1]);
		exit(EXIT_FAILURE);
	}

	// Open the file to make sure it exists
	FILE* file = fopen(argv[2], "r");
	if (file == NULL) {
		perror("Could not open file");
		exit(EXIT_FAILURE);
	}

	// Send the file to that user
	give_file(argv[1], argv[2], file);

	if (fclose(file) == -1) {
		perror("Could not close file");
		exit(EXIT_FAILURE);
	}
	return 0;
}

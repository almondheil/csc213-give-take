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

#include "communication.h"

void take_file(char * give_user) {
	// Find the fifo this transaction will occur through
	char * fifo_name = find_fifo_name(give_user, getenv("LOGNAME"));

	// Attempt to open it
	int fifo_fd = open(fifo_name, O_RDONLY);
	if (fifo_fd == -1) {
		perror("Failed to open fifo");
		exit(EXIT_FAILURE);
	}

	// Try to read the data through it
	filedata_t* data = recv_file(fifo_fd);
	if (data == NULL) {
		perror("Failed to receive file through FIFO");
		exit(EXIT_FAILURE);
	}

	// Refuse to overwrite the file if it already exists
	// TODO: It would be nice if the transfer could be attempted again...
	if (access(data->name, F_OK) == 0) {
		fprintf(stderr, "File \"%s\" already exists!\n", data->name);
		exit(EXIT_FAILURE);
	}

	// Open the file to write everything into it
	FILE* stream = fopen(data->name, "w");
	if (stream == NULL) {
		perror("Failed to open output file");
		exit(EXIT_FAILURE);
	}

	// Write the transferred data into the new file
	for (int i = 0; i < data->size; i++) {
		char ch = (char) data->data[i];
		fputc(ch, stream);
	}

	// Save and close the file
	if (fclose(stream)) {
		perror("Failed to close output file");
		exit(EXIT_FAILURE);
	}

	// Free malloc'd structures
	free(data->name);
	free(data->data);
	free(data);
	free(fifo_name);
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

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

// Send a file to a user. Returns when the sending is complete
// TODO: Document
void give_file(char * target_user, char * filename) {
	/* Prepare to send by storing the data of the file */

	// Open the file
	FILE * stream = fopen(filename, "r");
	if (stream == NULL) {
		perror("Could not read file");
		exit(EXIT_FAILURE);
	}

	// Set up a filedata_t with the right filename
	filedata_t * data = malloc(sizeof(filedata_t));
	data->name = filename;

	// Read the contents of the file into the data struct we have
	if (read_file_contents(data, stream) == -1) {
		fprintf(stderr, "Failed to read file contents\n");
		exit(EXIT_FAILURE);
	}

	// Close the file now, we have its data stored
	if (fclose(stream)) {
		perror("Could not close file");
		exit(EXIT_FAILURE);
	}

	/* Create a FIFO to send the file data through */

	// Determine the name to use
	char * fifo_name = find_fifo_name(getenv("LOGNAME"), target_user);

	// Make it a FIFO
	int rc = mknod(fifo_name, S_IFIFO | 0644, 0);
	if (rc == -1) {
		perror("Give failed to make FIFO");
		exit(EXIT_FAILURE);
	}

	// Open the FIFO. This call returns once we get a reader
	int fifo_fd = open(fifo_name, O_WRONLY);
	if (fifo_fd == -1) {
		perror("Give failed to open FIFO");
		free(data);
		remove(fifo_name);
		exit(EXIT_FAILURE);
	}

	// TODO: We do not check if the reader is the correct user.
	// For now we're just pretending it's okay
	// that might be something that has to stand in the final version but who knows

	// Send the file data through the FIFO
	send_file(fifo_fd, data);

	// Remove the FIFO to clean up after the communication
	remove(fifo_name);

	// Free malloc'd structures
	free(data->data);
	free(data);
	free(fifo_name);
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

	// Fork off a child process to do the work
	switch (fork()) {
		case -1:
			// Report if fork() had an error
			perror("Failed to create a daemon");
			exit(EXIT_FAILURE);
		case 0:
			// Child goes onwards in the code
			break;
		default:
			// Parent returns normally (does not wait)
			return 0;
	}

	// Detach from the parent process so we keep running even if they log out
	if (setsid() == -1) {
		perror("Failed to create new session");
		exit(EXIT_FAILURE);
	}

	// Then, give the file to the target user (and wait for it to go through)
	give_file(argv[1], argv[2]);
	return 0;
}

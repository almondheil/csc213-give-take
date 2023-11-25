#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <pwd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "message.h"
#include "socket.h"

void take_file(char * give_user, int fd) {
	// Try to read the data through it
	file_t* data = recv_file(fd);
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
}

int main(int argc, char ** argv) {
	// TODO: Later, I may want a case for just running with argc == 1 -- list out pending files
	if (argc != 2 && argc != 3) {
		printf("Usage: %s PORT [MACHINE]\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	// If the user trying to take from themselves, don't let them
	char * take_username = getenv("LOGNAME");
	if (take_username == NULL) {
		fprintf(stderr, "Could not determine your username");
		exit(EXIT_FAILURE);
	}
	// TODO: Later, don't allow taking from yourself
	// if (strcmp(take_username, argv[1]) == 0) {
	// 	fprintf(stderr, "Cannot take a file from yourself\n");
	// 	exit(EXIT_FAILURE);
	// }

	// TODO: localhost vs other machine processing
	unsigned short port = atoi(argv[1]);

	// TODO: Update away from localhost but yeah
	int socket_fd = socket_connect("localhost", port);
  if (socket_fd == -1) {
    perror("Failed to connect");
    exit(EXIT_FAILURE);
  }

	take_file(argv[1], socket_fd);

	return 0;
}

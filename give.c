#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "message.h"
#include "socket.h"

// TODO: Should this go here? AAAAAA
int read_file_contents(file_t * file_data, FILE* stream) {
	// Seek to the end of the file so we can get its size
	if (fseek(stream, 0, SEEK_END) != 0) {
		perror("Could not seek to end of file");
		return -1;
	}

	// Find the size
	file_data->size = ftell(stream);

	// Seek back to the start so we can read it
	if (fseek(stream, 0, SEEK_SET) != 0) {
		perror("Could not seek to start of file");
		return -1;
	}

	// Allocate space, and make sure everything fits
	file_data->data = malloc(file_data->size);
	if (file_data->data == NULL) {
		perror("Could not allocate space to store file contents");
		return -1;
	}

	// Read the file data into the malloced space
	if (fread(file_data->data, 1, file_data->size, stream) != file_data->size) {
		fprintf(stderr, "Failed to read the entire file\n");
		return -1;
	}

	return 0;
}

// Arguments needed to communicate with a client
typedef struct {
	int client_socket_fd;
	file_t* data;
} comm_args_t;

void* client_thread(void* arg) {
	comm_args_t* args = (comm_args_t*) arg;
	file_t* data = args->data;
	int client_socket_fd = args->client_socket_fd;

	while (true) {
		// Recieve a command that the client sends us


		// Send back some sort of confirmation or data

	}
}

// Send a file to a user. Returns when the sending is complete
// TODO: Document
int give_file(char * target_user, char * filename, int socket_fd) {
	/* Prepare to send by storing the data of the file */

	// Open the file
	FILE * stream = fopen(filename, "r");
	if (stream == NULL) {
		return -1;
	}

	// Set up a file_t with the right filename
	file_t * data = malloc(sizeof(file_t));
	data->name = filename;

	// Read the contents of the file into the data struct we have
	if (read_file_contents(data, stream) == -1) {
		return -1;
	}

	// Close the file now, we have its data stored
	if (fclose(stream)) {
		return -1;
	}

	// Accept new connections
	while (true) {
    int client_socket_fd = server_socket_accept(socket_fd);
    if (client_socket_fd == -1) {
			return -1;
    }

		comm_args_t args = {data, &client_socket_fd};

    // Spin up a thread to communicate with the client
    // TODO do we need thread data after the loop iteration (eg for joining?)
    pthread_t thread;
    if (pthread_create(&thread, NULL, client_thread,
                       &args)) {
      perror("Failed to create client communication thread");
      return -1;
    }
	}

	// TODO: We do not check if the reader is the correct user.
	// For now we're just pretending it's okay
	// that might be something that has to stand in the final version but who knows

	// Send the file data through the FIFO
	send_file(socket_fd, data);

	// Free malloc'd structures
	free(data->data);
	free(data);
	return 0;
}

/**
 * Determine whether a username belongs to a user on the system.
 * TODO: THIS SHOULD BE USABLE BY BOTH GIVE AND TAKE, BUT IN WHAT FILE?
 *
 * \param name A potential username
 * \return true if name belongs to a user, false otherwise
 */
bool user_exists(char * name) {
	struct passwd * user = getpwnam(name);
	return (user != NULL);
}

// Entry point to the program.
int main(int argc, char ** argv) {
	// Handle help text first
	if (argc == 2 && strcmp(argv[1], "--help") == 0) {
		// TODO: Write help text
		// print_help();
		exit(0);
	}

	// Handle actual program behavior
	if (argc != 3 && argc != 4) {
		// Case for definitely the wrong number of arguments

		fprintf(stderr, "Invalid syntax! See %s --help for usage.\n", argv[0]);
		exit(EXIT_FAILURE);
	} else if (argc == 3) {
		// Case for `give <user> <file>`

		// Check argv[1] is a user
		if (!user_exists(argv[1])) {
			fprintf(stderr, "User %s does not exist!\n", argv[1]);
			exit(EXIT_FAILURE);
		}

		// TODO: UNCOMMENT
		// // Check the user is not giving to themself
		// if (strcmp(getenv("LOGNAME"), argv[1]) == 0) {
		// 	fprintf(stderr, "Cannot give a file to yourself!\n");
		// 	exit(EXIT_FAILURE);
		// }

		// Check argv[2] is a file
		if (access(argv[2], F_OK) != 0) {
			fprintf(stderr, "File %s does not exist!\n", argv[2]);
			exit(EXIT_FAILURE);
		}

		// Open a port for the server

		// TODO: We need to somehow figue out how to tell the user
		// errors in a good and helpful way. Do we just let them die silently?
		// Maybe that's what we do if there are errors after the initial setup sequence

		// TODO: UNCOMMENT THIS TO TURN BACK ON DAEMON
		// // Fork off a child process to do the work
		// switch (fork()) {
		// 	case -1:
		// 		// Report if fork() had an error
		// 		perror("Failed to create a daemon");
		// 		exit(EXIT_FAILURE);
		// 	case 0:
		// 		// Child goes onwards in the code
		// 		break;
		// 	default:
		// 		// Parent does not wait for child
		// 		printf("File waiting on port %d\n", port);
		// 		return 0;
		// }

		// // Detach from the parent process so we keep running even if they log out
		// if (setsid() == -1) {
		// 	perror("Failed to create new session");
		// 	exit(EXIT_FAILURE);
		// }
	} else {
		// Case for `give -c <user> <port>`

		// Check argv[1] is "-c"
		if (strcmp(argv[1], "-c") != 0) {
			fprintf(stderr, "Expected argument -c, see %s --help for usage.\n", argv[0]);
			exit(EXIT_FAILURE);
		}

		// Check argv[2] is a user
		if (!user_exists(argv[2])) {
			fprintf(stderr, "User %s does not exist!\n", argv[1]);
			exit(EXIT_FAILURE);
		}
		// TODO: Connect to the port and make sure that's okay


		// TODO
	}

	// TODO: REMOVE THIS WHEN DAEMON IS BACK
	printf("File waiting on port %d\n", port);

	// Then, give the file to the target user (and wait for it to go through)
	int status = give_file(argv[1], argv[2], socket_fd);
	if (status == -1) {
		perror("Failed to give file");
		exit(EXIT_FAILURE);
	}

	// Close the socket once the transfer is complete
	close(socket_fd);

	return 0;
}

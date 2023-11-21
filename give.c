#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define FIFO_NAME "/home/almond/fifo"

// TODO: Remove
void test_give() {
	int rc = mknod(FIFO_NAME, S_IFIFO | 0644, 0);
	if (rc == -1) {
		perror("Give failed to make fifo");
		exit(EXIT_FAILURE);
	}

	printf("waiting for a reader\n");
	int fd = open(FIFO_NAME, O_WRONLY);
	if (fd == -1) {
		perror("Failed to open fifo");
		exit(EXIT_FAILURE);
	}

	printf("got a reader. sending some test data.\n");

	char * s = "123456";
	int num = write(fd, s, strlen(s));
	if (num == -1) {
		perror("Failed to write to fifo");
		exit(EXIT_FAILURE);
	}

	// remove the fifo once done
	remove(FIFO_NAME);
}

// TODO: Document
// must be freed by caller
char * make_fifo_name(char * target_user) {
	char * home = getenv("HOME");
	int fifo_name_len = strlen(home) + strlen("/.give-take-") + strlen(target_user) + 1;
	char * fifo_name = malloc(sizeof(char) * fifo_name_len);
	strcpy(fifo_name, home);
	strcat(fifo_name, "/.give-take-");
	strcat(fifo_name, target_user);
	return fifo_name;
}

// Open a file and write its contents into an fd.
// TODO: Document
int write_file_contents(int fifo_fd, int file_fd) {
	return -1;
}

// Send a file to a user. Returns when the sending is complete
// TODO: Document
int send_file(char * target_user, char * filepath, int file_fd) {
	// Determine a name for this fifo
	char * fifo_name = make_fifo_name(target_user);

	// Open that FIFO and check for any errors
	int rc = mknod(fifo_name, S_IFIFO | 0644, 0);
	if (rc == -1) {
		perror("Give failed to make FIFO");
		return -1;
	}

	// Open the FIFO. This call returns once we get a reader
	int fifo_fd = open(fifo_name, O_WRONLY);
	if (fifo_fd == -1) {
		perror("Give failed to open FIFO");
		remove(fifo_name);
		return -1;
	}

	// TODO: We do not check if the reader is the correct user.
	// For now we're just pretending it's okay, but it's really not.

	// Write the contents of the file to the FIFO
	rc = write_file_contents(fifo_fd, file_fd);
	if (rc == -1) {
		perror("Give failed to write file");
		remove(fifo_name);
		return -1;
	}

	// Remove the FIFO to clean up after the communication
	remove(FIFO_NAME);
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
	if (strcmp(give_username, argv[1]) == 0) {
		fprintf(stderr, "Error: Cannot give a file to yourself\n");
		exit(EXIT_FAILURE);
	}

	// Check that the user they are trying to send to exists
	struct passwd * to_user = getpwnam(argv[1]);
	if (to_user == NULL) {
		fprintf(stderr, "Error: Could not find the user %s\n", argv[1]);
		exit(EXIT_FAILURE);
	}

	// Open the file to make sure it exists
	int file_fd = open(argv[2], O_RDONLY);
	if (file_fd == -1) {
		perror("Could not open file");
		exit(EXIT_FAILURE);
	}

	// Send the file to that user
	send_file(argv[1], argv[2], file_fd);

	if (close(file_fd) == -1) {
		perror("Could not close file");
		exit(EXIT_FAILURE);
	}
	return 0;
}

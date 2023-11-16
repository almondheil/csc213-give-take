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

void test_take() {
	// take should not need to create a fifo right?
	// int rc = mknod(FIFO_NAME, S_IFIFO | 0644, 0);
	// if (rc == -1) {
	// 	perror("Take failed to make fifo");
	// 	exit(EXIT_FAILURE);
	// }

	printf("waiting for a writer\n");
	int fd = open(FIFO_NAME, O_RDONLY);
	if (fd == -1) {
		perror("Failed to open fifo");
		exit(EXIT_FAILURE);
	}

	printf("got a writer. accepting test data.\n");

	char s[300];
	int num = read(fd, s, 30);
	if (num == -1) {
		perror("Failed to write to fifo");
		exit(EXIT_FAILURE);
	}

	printf("got data: %s\n", s);
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
	if (strcmp(take_username, argv[1]) == 0) {
		fprintf(stderr, "Error: Cannot take a file from yourself\n");
		exit(EXIT_FAILURE);
	}

	// Check that the user they are trying to take from exists
	struct passwd * from_user = getpwnam(argv[1]);
	if (from_user == NULL) {
		fprintf(stderr, "Error: Could not find the user %s\n", argv[1]);
		exit(EXIT_FAILURE);
	}

	printf("Right now, the program would take files from the user %s\n", argv[1]);
	test_take();
	return 0;
}

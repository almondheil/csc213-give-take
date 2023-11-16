#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

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

	// TODO: Later on, check that argv[2] is a valid file when we try to open it

	printf("Right now, the program would give the file %s to the user %s\n", argv[2], argv[1]);
	return 0;
}

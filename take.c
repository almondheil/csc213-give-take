#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

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
	return 0;
}

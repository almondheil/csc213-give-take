#include <stdio.h>
#include <stdlib.h>

int main(int argc, char ** argv) {
	if (argc != 3) {
		printf("Usage: %s USER FILE\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	printf("This is the give program, it sure is running\n");
	return 0;
}

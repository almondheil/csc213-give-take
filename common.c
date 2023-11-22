#include <stdlib.h>
#include <string.h>

#include "common.h"

char * find_fifo_name(char* give_username, char* take_username) {
	// Malloc space for the name we want to use
	int fifo_name_len = strlen("/home/") + strlen(give_username) +
		strlen("/.give-take-") + strlen(take_username) + 1;
	char * fifo_name = malloc(sizeof(char) * fifo_name_len);

	// Copy all the parts into the string
	strcpy(fifo_name, "/home/");
	strcat(fifo_name, give_username);
	strcat(fifo_name, "/.give-take-");
	strcat(fifo_name, take_username);
	return fifo_name;
}

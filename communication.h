#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct {
	char* name;
	size_t size;
	uint8_t* data;
} filedata_t;

// TODO DOCUMENT
int read_file_contents(filedata_t * file_data, FILE* stream);

// TODO document
int send_file(int fifo_fd, filedata_t* file_data);

// TODO DOCUMENT
filedata_t* recv_file(int fifo_fd);

/**
 * Determine the name of a FIFO from the users involved in the
 * transaction.
 * TODO: May require more info down the line, this is bad for multiple
 * transactions and for
 *
 * \param give_username Username of the user giving a file
 * \param take_username Username of the user taking a file
 * \return The name a FIFO would have between those two users.
 */
char * find_fifo_name(char* give_username, char* take_username);

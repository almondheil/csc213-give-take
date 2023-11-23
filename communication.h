#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct {
	char* name;
	size_t size;
	uint8_t* data;
} filedata_t;

/**
 * Read the contents of a file stream into a filedata_t structure.
 * After this operation is complete, the file data is saved in memory and the
 * file can be closed.
 *
 * \param   file_data Struct to read data into. Will set `size` and `data` fields, but
 *          the caller is responsible for setting `name`.
 * \param   stream Open file with read access.
 * \return  0 if everything was read correctly, -1 otherwise
 */
int read_file_contents(filedata_t * file_data, FILE* stream);

/**
 * Send a file through an fd
 *
 * \param   fifo_fd File descriptor of a FIFO to write into
 * \param   file_data Filled out file data struct to be transferred
 * \return  0 if there were no errors, -1 otherwise
 */
int send_file(int fifo_fd, filedata_t* file_data);

/**
 * Receive a file through an fd
 *
 * \param   fifo_fd File descriptor of a FIFO to read from
 * \return  A malloc'd filedata struct of the message if transfer was completed,
 *          NULL if something went wrong.
 */
filedata_t* recv_file(int fifo_fd);

/**
 * Determine the name of a FIFO from the users involved in the
 * transaction.
 * TODO: May require more info down the line, this is bad for multiple
 * transactions and for security
 *
 * \param give_username Username of the user giving a file
 * \param take_username Username of the user taking a file
 * \return The name a FIFO would have between those two users.
 */
char * find_fifo_name(char* give_username, char* take_username);

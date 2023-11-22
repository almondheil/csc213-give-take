// max 1mb file basically? does that...work?
#define MAX_FILE_SIZE 1000000

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

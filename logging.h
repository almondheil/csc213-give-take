/**
 * logging.h
 *
 * Track outgoing gives so they can be checked from any computer.
 */

#pragma once

#define STATUS_FILE_NAME ".gives"
#define MAX_PATH_LEN 4096

/**
 * Add a give to the status file.
 *
 * \param file_name    The shortname of the file being given
 * \param target_user  The user the give is intended for.
 * \param host         The machine the give was started from.
 * \param port         The port the give is using.
 * \return             0 if everything went well, -1 on error
 */
int add_give_status(char *file_name, char *target_user, char *host,
                    unsigned int port);

/**
 * Remove a give from the status file.
 *
 * \param host  The hostname of the give to remove.
 * \param port  The port it was hosted on.
 * \return      1 if successfully removed, 0 if not removed but no errors, -1 on
 * error
 */
int remove_give_status(char *host, unsigned int port);

/**
 * Print the contents of the status file.
 */
void print_give_status();

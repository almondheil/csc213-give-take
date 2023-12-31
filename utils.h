/**
 * utils.h
 *
 * Common utilities used by both give and take
 */

#pragma once

/**
 * Shorten a pathname to just the name of a file.
 * Essentially turns "/path/to/file" into "file".
 *
 * \param path  Full path to file
 * \return      Pointer to the start of the shortname. Does not modify path.
 */
char* get_shortname(char* path);

/**
 * Parse connection info in the form server:port or port.
 *
 * \param in        Input string. Assumed to have the form server:port or port,
 *                  where port is an unsigned short integer value.
 * \param hostname  Output pointer to hostname. Must have enough space to hold
 *                  hostname, either "localhost" or whatever the user inputs
 * \param port      Output pointer to connection port.
 */
void parse_connection_info(char* in, char* hostname, unsigned short* port);

/**
 * Determine the username from the euid of the running process.
 *
 * \return Pointer to username if it exists, or NULL if it does not.
 */
char* get_username();

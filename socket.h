/**
 * socket.h
 *
 * Manage web sockets through a slightly easier interface.
 * Taken from networking exercise, and adapted to fit .c and .h file patterns
 */

#pragma once

/**
 * Create a new socket and connect to a server.
 *
 * \param server_name   A null-terminated string that specifies either the IP
 *                      address or host name of the server to connect to.
 * \param port          The port number the server should be listening on.
 *
 * \returns   A file descriptor for the connected socket, or -1 if there is an
 *            error. The errno value will be set by the failed POSIX call.
 */
int socket_connect(char* server_name, unsigned short port);

/**
 * Open a server socket that will accept TCP connections from any other machine.
 *
 * \param port    A pointer to a port value. If *port is greater than zero, this
 *                function will attempt to open a server socket using that port.
 *                If *port is zero, the OS will choose. Regardless of the method
 *                used, this function writes the socket's port number to *port.
 *
 * \returns       A file descriptor for the server socket. The socket has been
 *                bound to a particular port and address, but is not listening.
 *                In case of failure, this function returns -1. The value of
 *                errno will be set by the POSIX socket function that failed.
 */
int server_socket_open(unsigned short* port);

/**
 * Accept an incoming connection on a server socket.
 *
 * \param server_socket_fd  The server socket that should accept the connection.
 *
 * \returns   The file descriptor for the newly-connected client socket. In case
 *            of failure, returns -1 with errno set by the failed accept call.
 */
int server_socket_accept(int server_socket_fd);

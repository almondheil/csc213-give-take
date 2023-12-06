/**
 * message.h
 *
 * Send messages and requests as messages over the network.
 * Adapted from message.h used in in-class networking exercises.
 */

#pragma once

#include "filereader.h"

// Possible actions for a request
typedef enum {
  SEND_DATA,
  QUIT_SERVER,
} action_t;

// Action request, including requester username
typedef struct {
  char* username;
  action_t action;
} request_t;

/**
 * Send a file through a socket
 *
 * \param   sock_fd File descriptor of the socket to send to
 * \param   file_data Filled out file data struct to be transferred
 * \return  0 if there were no errors, -1 otherwise
 */
int send_file(int sock_fd, file_t* file_data);

/**
 * Receive a file through a socket
 *
 * \param   sock_fd File descriptor of the socket to read from
 * \return  A malloc'd filedata struct of the message if transfer was completed,
 *          NULL if something went wrong.
 */
file_t* recv_file(int sock_fd);

/**
 * Send a request through a socket
 *
 * \param   sock_fd File descriptor of the socket to send to
 * \param   req Fiilled out request struct to be transferred
 * \return  0 if there were no errors, -1 otherwise
 */
int send_request(int sock_fd, request_t* req);

/**
 * Receive a request through a socket
 *
 * \param   sock_fd File descriptor of the socket to read from
 * \return  A malloc'd request struct of the message if transfer was completed,
 *          NULL if something went wrong.
 */
request_t* recv_request(int sock_fd);

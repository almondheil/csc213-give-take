#include <stdint.h>

// File name and contents
typedef struct {
	char* name;     //< filename
	size_t size;    //< size of file in bytes
	uint8_t* data;  //< file data bytes
} file_t;

// Possible actions for a request
typedef enum {
	DATA,  //< send file data
	QUIT,  //< stop hosting
} action_t;

// Action request, including requester username
typedef struct {
	char* name;       //< username of requester
	action_t action;  //< requested action
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

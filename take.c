#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "message.h"
#include "socket.h"
#include "utils.h"

/**
 * Take a file through a network socket.
 *
 * \param socket_fd  File descriptor of the open network socket.
 * \param save_name  Name to save the file under, or NULL if the default name
 *                   should be used.
 */
void take_file(int socket_fd, char* save_name) {
  // Send a request for the data to the server side
  request_t req;
  req.username = get_username();
  req.action = SEND_DATA;
  int rc = send_request(socket_fd, &req);
  if (rc == -1) {
    perror("Failed to send file request");
    exit(EXIT_FAILURE);
  }

  // Try to receive the data now
  file_t* file = recv_file(socket_fd);
  if (file == NULL) {
    if (errno != 0) {
      perror("Failed to receive file");
    } else {
      fprintf(stderr, "You don't have permission to take that file!\n");
    }
    exit(EXIT_FAILURE);
  }

  // If a save name was provided, overwrite the default it was sent with
  char* original_name = file->name;
  if (save_name != NULL) {
    file->name = save_name;
  }

  // Attempt to write the file to the current directory
  if (write_file("./", file) == -1) {
    free_file(file);
    return;
  }

  // Once we successfully save the file, tell the server to quit
  req.action = QUIT_SERVER;
  rc = send_request(socket_fd, &req);
  if (rc == -1) {
    perror("Failed to send quit request");
    free_file(file);
    exit(EXIT_FAILURE);
  }

  // Announce that we got the transfer across
  printf("Successfully took %s\n", file->name);

  // Swap in the original name of the file in case we changed it when
  // writing the file to disk (needed to make free_file clean up properly)
  file->name = original_name;

  // Free malloc'd structures
  free_file(file);
}

int main(int argc, char** argv) {
  // Make sure there are the right number of parameters
  if (argc != 2 && argc != 3) {
    printf("Usage: %s [HOST:]PORT [NAME]    (take file)\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  // If the user trying to take from themselves, don't let them
  char* take_username = get_username();
  if (take_username == NULL) {
    fprintf(stderr, "Could not determine your username");
    exit(EXIT_FAILURE);
  }

  // Make space for the worst case hostname length. argv[1] cannot entirely be a
  // hostname, so this allocates extra space, but that's okay.
  // This is also long enough to fully contain "localhost" no matter what.
  char hostname[strlen(argv[1]) + strlen(".cs.grinnell.edu")];

  // Parse a port and hostname from argv[1]
  unsigned short port = 0;
  parse_connection_info(argv[1], hostname, &port);

  if (port == 0) {
    fprintf(stderr, "Could not parse port from argument %s!\n", argv[2]);
    exit(EXIT_FAILURE);
  }

  // Attempt to connect to that socket
  int socket_fd = socket_connect(hostname, port);
  if (socket_fd == -1) {
    perror("Failed to connect");
    exit(EXIT_FAILURE);
  }

  // Take the file from that socket
  // If a 3rd arg was provided, save under that name
  if (argc == 3) {
    take_file(socket_fd, argv[2]);
  } else {
    take_file(socket_fd, NULL);
  }

  // Close the socket before we exit
  close(socket_fd);
  return 0;
}

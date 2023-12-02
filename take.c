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
 */
void take_file(int socket_fd) {
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
  file_t *file = recv_file(socket_fd);
  if (file == NULL) {
    perror("Failed to receive file");
    exit(EXIT_FAILURE);
  }

  // Attempt to write the file to the current directory
  // TODO: more glamorous way to call write_file? pwd?
  if (write_file("./", file) == -1) {
    printf("OYYY TODO ERROR MESSAGE GOOD\n");
    return; // TODO FREE STUFF TODO GOOD ERROR MESSAGE
  }

  // Once we successfully save the file, tell the server to quit
  req.action = QUIT_SERVER;
  rc = send_request(socket_fd, &req);
  if (rc == -1) {
    perror("Failed to send quit request");
    exit(EXIT_FAILURE);
  }

  // Announce that we got the transfer across
  printf("Successfully took %s\n", file->name);

  // Free malloc'd structures
  free_file(file);
}

// Entry point to the program.
int main(int argc, char **argv) {
  // Make sure there are the right number of parameters
  if (argc != 2) {
    printf("Usage: %s [HOST:]PORT\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  // If the user trying to take from themselves, don't let them
  char *take_username = get_username();
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

  // Attempt to connect to that socket
  int socket_fd = socket_connect(hostname, port);
  if (socket_fd == -1) {
    perror("Failed to connect");
    exit(EXIT_FAILURE);
  }

  // Take the file from that socket
  take_file(socket_fd);

  // Close the socket before we exit
  close(socket_fd);
  return 0;
}

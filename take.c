#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <pwd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "message.h"
#include "socket.h"
#include "utils.h"

void take_file(int fd) {
  // Send a request for the data to the server side
  request_t req;
  req.name = getenv("LOGNAME");
  req.action = DATA;
  int rc = send_request(fd, &req);
  if (rc == -1) {
    perror("Failed to send file request");
    exit(EXIT_FAILURE);
  }

  // Try to receive the data now
  file_t *data = recv_file(fd);
  if (data == NULL) {
    perror("Failed to receive file");
    exit(EXIT_FAILURE);
  }

  // Refuse to overwrite the file if it already exists
  // TODO: It would be nice if the transfer could be attempted again...
  if (access(data->name, F_OK) == 0) {
    fprintf(stderr, "File \"%s\" already exists! Cancelling transfer.\n",
            data->name);
    exit(EXIT_FAILURE);
  }

  // Send the request to quit now that we're ready
  req.action = QUIT;
  rc = send_request(fd, &req);
  if (rc == -1) {
    perror("Failed to send quit request");
    exit(EXIT_FAILURE);
  }

  // Open the file to write everything into it
  FILE *stream = fopen(data->name, "w");
  if (stream == NULL) {
    perror("Failed to open output file");
    exit(EXIT_FAILURE);
  }

  // Write the transferred data into the new file
  for (int i = 0; i < data->size; i++) {
    char ch = (char)data->data[i];
    fputc(ch, stream);
  }

  // Save and close the file
  if (fclose(stream)) {
    perror("Failed to close output file");
    exit(EXIT_FAILURE);
  }

  // Free malloc'd structures
  free(data->name);
  free(data->data);
  free(data);
}

int main(int argc, char **argv) {
  // Handle help text first
  if (argc == 2 && strcmp(argv[1], "--help") == 0) {
    // TODO: Write help text
    // print_help();
    exit(0);
  }

  // TODO: Later, I may want a case for just running with argc == 1 -- list out
  // pending files
  if (argc != 2) {
    printf("Usage: %s [HOST:]PORT\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  // If the user trying to take from themselves, don't let them
  char *take_username = getenv("LOGNAME");
  if (take_username == NULL) {
    fprintf(stderr, "Could not determine your username");
    exit(EXIT_FAILURE);
  }
  // // TODO: Later, don't allow taking from yourself
  // if (strcmp(take_username, argv[1]) == 0) {
  // 	fprintf(stderr, "Cannot take a file from yourself\n");
  // 	exit(EXIT_FAILURE);
  // }

  // Make space for the worst case hostname length. argv[1] cannot entirely be a
  // hostname, so this allocates extra space, but that's okay.
  // This is also long enough to fully contain "localhost" no matter what.
  char hostname[strlen(argv[1]) + strlen(".cs.grinnell.edu")];

  // Parse a port and hostname from that
  unsigned short port = 0;
  parse_connection_info(argv[1], hostname, &port);

  // TODO: Update away from localhost but yeah
  int socket_fd = socket_connect("localhost", port);
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

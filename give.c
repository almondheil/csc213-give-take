#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "message.h"
#include "socket.h"
#include "utils.h"

// Arguments needed to communicate with a client in a thread
typedef struct {
  int client_socket_fd;  //< socket fd of connected client
  file_t *data;          //< pointer to file data
  char *target_username; //< username of the intended client
  char *owner_username;  //< username of the user who initialized the get
} comm_args_t;

/**
 * Receive requests from a client and act on them.
 *
 * \param arg  Pointer to a malloc'd comm_args_t struct filled out with correct
 *             data. Will be free'd before this thread exits to avoid leaks.
 * \return     NULL, only here because threads must return something
 */
void *receive_client_requests(void *arg) {
  // Unpack args struct passed in
  comm_args_t *args = (comm_args_t *)arg;
  int client_socket_fd = args->client_socket_fd;
  file_t *data = args->data;
  char *target_username = args->target_username;
  char *owner_username = args->owner_username;

  while (true) {
    // Recieve a request that the client sends us
    request_t *req = recv_request(client_socket_fd);
    if (req == NULL) {
      free(args);

      // Close the client socket--something went wrong
      close(client_socket_fd);

      // Return, stopping this thread
      return NULL;
    }

    // Terminate the server if owner sends CANCEL or target sends DONE
    if ((req->action == QUIT_SERVER &&
         strcmp(req->username, owner_username) == 0) ||
        (req->action == QUIT_SERVER &&
         strcmp(req->username, target_username) == 0)) {
      free(args);
      free(req->username);
      free(req);

      // Close the client socket--something went wrong
      close(client_socket_fd);

      // Exit, stopping ALL threads
      exit(EXIT_SUCCESS);
    }

    // Send the data if the target sends SEND_DATA
    else if (req->action == SEND_DATA &&
             strcmp(req->username, target_username) == 0) {
      int rc = send_file(client_socket_fd, data);
      if (rc == -1) {
        free(args);
        free(req->username);
        free(req);

        // Close the client socket--something went wrong
        close(client_socket_fd);

        // Return, stopping this thread
        return NULL;
      }
    }

    // Otherwise, disconnect from the client
    else {
      free(args);
      free(req->username);
      free(req);

      // Close the client socket since they're not authenticated
      close(client_socket_fd);

      // Exit, stopping ALL threads
      return NULL;
    }

    // Free the request we got
    free(req->username);
    free(req);
  }
}

/**
 * Give a file to a target user through a network socket.
 *
 * \param target_username  User to send the file.
 * \param file_path        Full path to the file.
 * \param socket_fd        Network socket to send through.
 * \return                 0 if there are no errors, -1 if there are errors.
 *                         Sets errno on failure.
 */
int host_file(char *restrict target_username, char *restrict file_path,
              int socket_fd) {
  // Read that file into memory
  file_t *file = read_file(file_path);

  // Accept new connections while the server is running
  while (true) {
    int client_socket_fd = server_socket_accept(socket_fd);
    if (client_socket_fd == -1) {
      perror("Failed to accept new connection");
      return -1;
    }

    // Set up args for this thread
    // They will be freed when the thread exits
    comm_args_t *args = malloc(sizeof(comm_args_t));
    args->client_socket_fd = client_socket_fd;
    args->data = file;
    args->target_username = target_username;
    args->owner_username = get_username();

    // Spin up a thread to communicate with this client
    pthread_t thread;
    if (pthread_create(&thread, NULL, receive_client_requests, args)) {
      perror("Failed to create client communication thread");
      return -1;
    }
  }

  // Free malloc'd structures
  free_file(file);
  return 0;
}

/**
 * Print the usage of this program
 *
 * \param prog_name  The program name to include in usage help
 */
void print_usage(char *prog_name) {
  fprintf(stderr, "Usage: %s USER FILE\n", prog_name);
  fprintf(stderr, "       %s -c [HOST:]PORT\n", prog_name);
}

// Entry point to the program.
int main(int argc, char **argv) {

  /*
   * Invalid usage
   */
  if (argc != 3) {
    // They definitely gave the wrong number of arguments
    print_usage(argv[0]);
    exit(EXIT_FAILURE);
  }

  /*
   * give -c [HOST:]PORT
   */
  else if (strcmp(argv[1], "-c") == 0) {
    // Check argv[1] is "-c",otherwise the arguments were malformed
    if (strcmp(argv[1], "-c") != 0) {
      print_usage(argv[0]);
      exit(EXIT_FAILURE);
    }

    // Make space for the worst case hostname length. argv[1] cannot entirely be
    // a hostname, so this allocates extra space, but that's okay. This is also
    // long enough to fully contain "localhost" no matter what.
    char hostname[strlen(argv[2]) + strlen(".cs.grinnell.edu")];

    // Parse a port and hostname from that
    unsigned short port = 0;
    parse_connection_info(argv[2], hostname, &port);

    // Connect to the port
    int socket_fd = socket_connect(hostname, port);
    if (socket_fd == -1) {
      perror("Failed to connect");
      exit(EXIT_FAILURE);
    }

    // Cancel the give
    request_t req;
    req.username = get_username();
    req.action = QUIT_SERVER;
    int rc = send_request(socket_fd, &req);
    if (rc == -1) {
      perror("Failed to send quit request");
      exit(EXIT_FAILURE);
    }

    // Announce that everything went okay
    printf("Successfully cancelled give\n");

    // Close the socket before we exit
    close(socket_fd);
  }

  /*
   * give USER FILE
   */
  else {
    // Check argv[1] is a user
    if (!user_exists(argv[1])) {
      fprintf(stderr, "User %s does not exist!\n", argv[1]);
      exit(EXIT_FAILURE);
    }

    // Check argv[2] exists
    if (access(argv[2], F_OK) != 0) {
      fprintf(stderr, "File %s does not exist!\n", argv[2]);
      exit(EXIT_FAILURE);
    }

    // Check argv[2] is a supported type of file, not something else
    struct stat st;
    if (stat(argv[2], &st) == -1) {
      perror("Could not stat file");
      exit(EXIT_FAILURE);
    }
    if (!S_ISREG(st.st_mode) && !S_ISDIR(st.st_mode)) {
      fprintf(stderr, "Can only give a regular file or a directory.\n");
      exit(EXIT_FAILURE);
    }

    // Open a port for the server
    unsigned short port = 0;
    int server_socket_fd = server_socket_open(&port);
    if (server_socket_fd == -1) {
      perror("Server socket was not opened");
      exit(EXIT_FAILURE);
    }

    // Start listening for connections, with a maximum of one queued connection
    if (listen(server_socket_fd, 1)) {
      perror("listen failed");
      exit(EXIT_FAILURE);
    }

    // Fork off a child process to do the work
    switch (fork()) {
    case -1:
      // Report if fork() had an error
      perror("Failed to create a daemon");
      exit(EXIT_FAILURE);
    case 0:
      // Child goes onwards in the code
      break;
    default:
      // Parent does not wait for child
      printf("Server listening on port %u\n", port);
      return 0;
    }

    // Detach from the parent process so we keep running even if they log out
    if (setsid() == -1) {
      perror("Failed to create new session");
      exit(EXIT_FAILURE);
    }

    // TODO: Mark this give in the owner's ~/.gives file
    // info we need: target user, filename, port number
    // pid may also be useful? who knows

    // Give the user that file.
    int rc = host_file(argv[1], argv[2], server_socket_fd);
    if (rc == -1) {
      // host_file prints its own (more descriptive) error messages
      exit(EXIT_FAILURE);
    }

    // TODO: Remove this give from the owner's ~/.gives file

    // Close the socket once the transfer is complete
    close(server_socket_fd);
  }

  return 0;
}

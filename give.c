#include <pthread.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "logging.h"
#include "message.h"
#include "socket.h"
#include "utils.h"

// Global variables to track this give's info
#define MAX_HOSTNAME_LEN 128
unsigned short give_server_port = 0;
char give_host[MAX_HOSTNAME_LEN];

// Arguments needed to communicate with a client in a thread
typedef struct {
  int client_socket_fd;
  file_t* data;
  char* target_username;
  char* owner_username;
} comm_args_t;

/**
 * Receive requests from a client and act on them.
 *
 * \param arg  Pointer to a malloc'd comm_args_t struct filled out with correct
 *             data. Will be free'd before this thread exits to avoid leaks.
 * \return     NULL, only here because threads must return something
 */
void* receive_client_requests(void* arg) {
  // Unpack args struct passed in
  comm_args_t* args = (comm_args_t*)arg;
  int client_socket_fd = args->client_socket_fd;
  file_t* data = args->data;
  char* target_username = args->target_username;
  char* owner_username = args->owner_username;

  while (true) {
    // Recieve a request that the client sends us
    request_t* req = recv_request(client_socket_fd);
    if (req == NULL) {
      free(args);

      // Close the client socket--something went wrong
      close(client_socket_fd);

      // Return, stopping this thread
      return NULL;
    }

    // Terminate the server if owner sends CANCEL or target sends DONE
    if ((req->action == QUIT_SERVER && strcmp(req->username, owner_username) == 0) ||
        (req->action == QUIT_SERVER && strcmp(req->username, target_username) == 0)) {
      free(args);
      free(req->username);
      free(req);

      // Close the client socket
      close(client_socket_fd);

      // Remove this give from the status file
      remove_give_status(give_host, give_server_port);

      // Exit, stopping ALL threads
      exit(EXIT_SUCCESS);
    }

    // Send the data if the target sends SEND_DATA
    else if (req->action == SEND_DATA && strcmp(req->username, target_username) == 0) {
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
 * \param file             File stored in memory.
 * \param socket_fd        Network socket to send through.
 * \return                 0 if there are no errors, -1 if there are errors.
 *                         Sets errno on failure.
 */
int host_file(char* restrict target_username, file_t* file, int socket_fd) {
  // Accept new connections while the server is running
  while (true) {
    int client_socket_fd = server_socket_accept(socket_fd);
    if (client_socket_fd == -1) {
      perror("Failed to accept new connection");
      return -1;
    }

    // Set up args for this thread
    // They will be freed when the thread exits
    comm_args_t* args = malloc(sizeof(comm_args_t));
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

void print_usage(char* prog_name) {
  fprintf(stderr, "Usage: %s USER FILE\n", prog_name);
  fprintf(stderr, "       %s USER DIRECTORY\n", prog_name);
  fprintf(stderr, "       %s -c [HOST:]PORT\n", prog_name);
  fprintf(stderr, "       %s --status\n", prog_name);
}

int main(int argc, char** argv) {
  /*
   * First, parse the arguments
   */

  // Hold parsed argument info
  enum {STATUS, CANCEL, GIVE} mode;

  // args for cancel
  // cancel_host is long enough to hold any hostname
  char* cancel_host = NULL;
  unsigned short cancel_port = 0;

  // args for give, can be pointers as they come straight from argv
  char* give_user = NULL;
  char* give_path = NULL;
  // TODO maybe these? give_host and give_server_port

  if (argc == 2 && strcmp(argv[1], "--status") == 0) {
    // give --status
    mode = STATUS;
  }
  else if (argc == 3 && strcmp(argv[1], "-c") == 0) {
    // give -c [HOST]:PORT
    mode = CANCEL;

    // Allocate enough space in cancel_host to hold the hostname
    // (plus some extra space but that waste is okay)
    cancel_host = malloc(sizeof(char) * (strlen(argv[2]) + strlen(".cs.grinnell.edu") + 1));
    if (cancel_host == NULL) {
      perror("Failed to allocate space for hostname");
      exit(EXIT_FAILURE);
    }

    // Attempt to parse connection info from argv[2]
    parse_connection_info(argv[2], cancel_host, &cancel_port);
    if (cancel_port == 0) {
      fprintf(stderr, "Failed to parse port!\n");
      exit(EXIT_FAILURE);
    }
  }
  else if (argc == 3) {
    // give USER PATH
    mode = GIVE;
    give_user = argv[1];
    give_path = argv[2];

    // Check give_user is a real user
    // note: we are assuming the same user exists on the taking system. true on mathlan
    if (getpwnam(give_user) == NULL) {
      fprintf(stderr, "User %s does not exist!\n", give_user);
      exit(EXIT_FAILURE);
    }

    // Check give_path exists
    if (access(give_path, F_OK) != 0) {
      fprintf(stderr, "File %s does not exist!\n", give_path);
      exit(EXIT_FAILURE);
    }

    // If there is a / on the end of the give_path, trim it off
    int len = strlen(give_path);
    if (give_path[len-1] == '/') {
      give_path[len-1] = '\0';
    }

    // Store our hostname in the global var (something.cs.grinnell.edu)
    // TODO: later, I wanna have these be local and maybe just pass them to each thread as needed. ugh
    if (gethostname(give_host, MAX_HOSTNAME_LEN) == -1) {
      perror("Failed to get hostname");
      exit(EXIT_FAILURE);
    }

    // Trim the hostname down from something.cs.grinnell.edu -> something
    // (yes this means like 90% of the buffer is just unused, whatever)
    char* first_dot = strchr(give_host, '.');
    if (first_dot != NULL) {
      *first_dot = '\0';
    }
  } else {
    // wrong number of arguments
    print_usage(argv[0]);
    exit(EXIT_FAILURE);
  }

  /*
   * Then, act on the parsed arguments
   */
  if (mode == STATUS) {
      print_give_status();
      exit(0);
  } else if (mode == CANCEL) {

    // Connect to the port
    int socket_fd = socket_connect(cancel_host, cancel_port);
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
    printf("Successfully cancelled give.\n");

    // Close the socket before we exit
    free(cancel_host);
    close(socket_fd);
  } else if (mode == GIVE) {
    // Open a server, and store the port globally
    int server_socket_fd = server_socket_open(&give_server_port);
    if (server_socket_fd == -1) {
      perror("Failed to open server socket");
      exit(EXIT_FAILURE);
    }

    // Start listening for connections on the server
    if (listen(server_socket_fd, 1)) {
      perror("Failed to listen");
      exit(EXIT_FAILURE);
    }

    // Attempt to read the file into memory now.
    // If there's an error, we want to know before daemonizing
    file_t* file = malloc(sizeof(file_t));
    if (file == NULL) {
      perror("Failed to allocate file struct");
      exit(EXIT_FAILURE);
    }
    if(read_file(give_path, file) == -1) {
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
        free_file(file);
        printf("Server listening on port %u\n", give_server_port);
        exit(0);
    }

    // Detach from the parent process so we keep running even if they log out
    if (setsid() == -1) {
      perror("Failed to create new session");
      exit(EXIT_FAILURE);
    }

    // Log that we are giving this file
    add_give_status(give_path, argv[1], give_host, give_server_port);

    // Host the file until somebody quits the server
    // This function does not exit on success, but it cleans up after itself
    int rc = host_file(argv[1], file, server_socket_fd);
    if (rc == -1) {
      exit(EXIT_FAILURE);
    }
  }
}

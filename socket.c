#include "socket.h"

#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int socket_connect(char* server_name, unsigned short port) {
  // Look up the server by name
  struct hostent* server = gethostbyname(server_name);
  if (server == NULL) {
    // Set errno, since gethostbyname does not
    errno = EHOSTDOWN;
    return -1;
  }

  // Open a socket
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd == -1) {
    return -1;
  }

  // Set up an address
  struct sockaddr_in addr = {
      .sin_family = AF_INET,   // This is an internet socket
      .sin_port = htons(port)  // Connect to the appropriate port number
  };

  // Copy the server address info returned by gethostbyname into the address
  memcpy(&addr.sin_addr.s_addr, server->h_addr, server->h_length);

  // Connect to the web server
  if (connect(fd, (struct sockaddr*)&addr, sizeof(struct sockaddr_in))) {
    close(fd);
    return -1;
  }

  return fd;
}

int server_socket_open(unsigned short* port) {
  // Create a server socket. Return if there is an error.
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd == -1) {
    return -1;
  }

  // Set up the server socket to listen
  struct sockaddr_in addr = {
      .sin_family = AF_INET,          // This is an internet socket
      .sin_addr.s_addr = INADDR_ANY,  // Listen for connections from any client
      .sin_port = htons(*port)        // Use the specified port (may be zero)
  };

  // Bind the server socket to the address. Return if there is an error.
  if (bind(fd, (struct sockaddr*)&addr, sizeof(struct sockaddr_in))) {
    close(fd);
    return -1;
  }

  // Get information about the new socket
  socklen_t addrlen = sizeof(struct sockaddr_in);
  if (getsockname(fd, (struct sockaddr*)&addr, &addrlen)) {
    close(fd);
    return -1;
  }

  // Read out the port information for the socket. If *port was zero, the OS
  // will select a port for us. This tells the caller which port was chosen.
  *port = ntohs(addr.sin_port);

  // Return the server socket file descriptor
  return fd;
}

int server_socket_accept(int server_socket_fd) {
  // Create a struct to record the connected client's address
  struct sockaddr_in client_addr;
  socklen_t client_addr_len = sizeof(struct sockaddr_in);

  // Block until we receive a connection or failure
  int client_socket_fd = accept(server_socket_fd, (struct sockaddr*)&client_addr, &client_addr_len);

  // Did something go wrong?
  if (client_socket_fd == -1) {
    return -1;
  }

  return client_socket_fd;
}

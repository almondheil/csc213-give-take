// message.c
// Modified from message.c used in the networking exercise

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "filereader.h"
#include "message.h"

int send_file(int sock_fd, file_t *file) {
  // Send the type of the file
  if (write(sock_fd, &file->type, sizeof(filetype)) != sizeof(filetype)) {
    return -1;
  }

  // Send the length of the name
  size_t name_len = sizeof(char) * strlen(file->name);
  if (write(sock_fd, &name_len, sizeof(size_t)) != sizeof(size_t)) {
    return -1;
  }

  // Send file->size (either data size, or number of entries
  if (write(sock_fd, &file->size, sizeof(size_t)) != sizeof(size_t)) {
    return -1;
  }

  // Send the actual name
  size_t bytes_written = 0;
  while (bytes_written < name_len) {
    // Write the remaining message
    ssize_t rc = write(sock_fd, file->name + bytes_written,
        name_len - bytes_written);

    // if the write failed, there was an error
    if (rc <= 0) {
      return -1;
    }

    bytes_written += rc;
  }

  // Send the file contents over the network, depending on type
  if (file->type == F_REG) {
    // Regular files need only send their data across

    bytes_written = 0;
    while (bytes_written < file->size) {
      // write the remaining message
      ssize_t rc = write(sock_fd, file->contents.data + bytes_written,
                         file->size - bytes_written);

      // if the write failed, there was an error
      if (rc <= 0)
        return -1;

      bytes_written += rc;
    }
  } else {
    // For directories, recursively call send_file on each entry
    for (int i = 0; i < file->size; i++) {
      if (send_file(sock_fd, file->contents.entries[i]) == -1) {
        return -1;
      }
    }
  }

  return 0;
}

file_t *recv_file(int sock_fd) {
  // Read the type of the file
  filetype type;
  if (read(sock_fd, &type, sizeof(filetype)) != sizeof(filetype)) {
    return NULL;
  }

  // Read the length of the filename
  size_t filename_len;
  if (read(sock_fd, &filename_len, sizeof(size_t)) != sizeof(size_t)) {
    return NULL;
  }

  // Read the size of the file
  size_t data_size;
  if (read(sock_fd, &data_size, sizeof(size_t)) != sizeof(size_t)) {
    return NULL;
  }

  // Create space to store the received file
  file_t *file = malloc(sizeof(file_t));
  file->size = data_size;
  file->type = type;

  // Make space to store the filename
  file->name = malloc(filename_len + 1);
  if (file->name == NULL) {
    perror("could not allocate space to receive filename");
    free_file(file);
    return NULL;
  }

  // Read the filename of the file
  size_t bytes_read = 0;
  while (bytes_read < filename_len) {
    ssize_t rc = read(sock_fd, file->name + bytes_read,
        filename_len - bytes_read);

    if (rc <= 0) {
      free_file(file);
      return NULL;
    }

    bytes_read += rc;
  }
  file->name[filename_len] = '\0';

  // Read the contents of the file
  if (file->type == F_REG) {
    // For a regular file, read into file->contents.data

    // Make space to store the file contents
    file->contents.data = malloc(data_size);
    if (file->contents.data == NULL) {
      perror("failed to allocate space to receive file data");
      free_file(file);
      return NULL;
    }

    // Read the contents into our file struct
    bytes_read = 0;
    while (bytes_read < data_size) {
      ssize_t rc = read(sock_fd, file->contents.data + bytes_read,
                        data_size - bytes_read);

      if (rc <= 0) {
        free_file(file);
        return NULL;
      }

      bytes_read += rc;
    }
  } else {
    // For a directory, make space for file->size number of entries
    file->contents.entries = malloc(sizeof(file_t *) * file->size);

    // Then recursively receive each of those entries
    for (int i = 0; i < file->size; i++) {
      file->contents.entries[i] = recv_file(sock_fd);
      if (file->contents.entries[i] == NULL) {
        free_file(file);
        return NULL;
      }
    }
  }

  // All went well? Return that file.
  return file;
}

int send_request(int sock_fd, request_t *req) {
  // Send how long the name is
  size_t name_len = sizeof(char) * strlen(req->username);
  if (write(sock_fd, &name_len, sizeof(size_t)) != sizeof(size_t)) {
    return -1;
  }

  // Send the name over
  size_t bytes_written = 0;
  while (bytes_written < name_len) {
    // write the remaining message
    ssize_t rc = write(sock_fd, req->username + bytes_written,
        name_len - bytes_written);

    // if the write failed, there was an error
    if (rc <= 0)
      return -1;

    bytes_written += rc;
  }

  // Send the request value
  if (write(sock_fd, &req->action, sizeof(action_t)) != sizeof(action_t)) {
    return -1;
  }

  return 0;
}

request_t *recv_request(int sock_fd) {
  // Read the length of the name
  size_t name_len;
  if (read(sock_fd, &name_len, sizeof(size_t)) != sizeof(size_t)) {
    return NULL;
  }

  // Create a struct to store the values we'll receive
  request_t *req = malloc(sizeof(request_t));
  req->username = malloc(name_len + 1);

  // Read the name
  size_t bytes_read = 0;
  while (bytes_read < name_len) {
    ssize_t rc = read(sock_fd, req->username + bytes_read,
        name_len - bytes_read);

    if (rc <= 0) {
      free(req->username);
      free(req);
      return NULL;
    }

    bytes_read += rc;
  }

  // Null terminate the name
  req->username[name_len] = '\0';

  // Read the requested action
  if (read(sock_fd, &req->action, sizeof(action_t)) != sizeof(action_t)) {
    free(req->username);
    free(req);
    return NULL;
  }

  // Return the request now that we've read all its data
  return req;
}

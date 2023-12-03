#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "logging.h"

/**
 * Get an absolute path to the status file of this user.
 *
 * \return  Pointer to the path. Must be freed by the caller.
 */
char *status_file_path() {
  char *home_path = getenv("HOME");
  char *status_file_path = malloc(sizeof(char) *
      (strlen(home_path) + strlen("/") + strlen(STATUS_FILE_NAME) + 1));
  if (status_file_path == NULL) {
    return NULL;
  }

  strcpy(status_file_path, home_path);
  strcat(status_file_path, "/");
  strcat(status_file_path, STATUS_FILE_NAME);

  return status_file_path;
}

int add_give_status(char *file_name, char *target_user, char *host,
                    unsigned int port) {
  char *path = status_file_path();
  if (path == NULL) {
    perror("failed to allocate space for status file name");
    return -1;
  }

  // Open the status file
  FILE *stream = fopen(path, "a");
  if (stream == NULL) {
    perror("failed to open status file");
    free(path);
    return -1;
  }

  // We're done with path now no matter what
  free(path);

  // Write the data to the file, space separated for ease of use
  fprintf(stream, "%s %d %s %s\n", host, port, file_name, target_user);

  // Save and close the file
  if (fclose(stream)) {
    perror("failed to close status file");
    return -1;
  }

  return 0;
}

int remove_give_status(char *host, unsigned int port) {
  // Locate and open the original file
  char *path = status_file_path();
  if (path == NULL) {
    perror("failed to allocate space for status file name");
    return -1;
  }
  FILE *original = fopen(path, "r");
  if (original == NULL) {
    perror("failed to open status file");
    free(path);
    return -1;
  }

  // Construct a path to a tempfile we can use
  char *temp_path = strdup(path);
  temp_path = realloc(temp_path, sizeof(char) *
      (strlen(temp_path) + strlen(".tmp") + 1));
  strcat(temp_path, ".tmp");

  // Open that tempfile too, but in write mode
  FILE *copy = fopen(temp_path, "w");
  if (copy == NULL) {
    perror("failed to open temporary status file");
    free(path);
    return -1;
  }

  // Copy all lines into copy, EXCEPT the one we want to remove
  size_t sz = 0;
  char *line = NULL;
  ssize_t chars_read;
  while ((chars_read = getline(&line, &sz, original)) != -1) {
    char *to_modify = strdup(line);
    if (to_modify == NULL) {
      perror("failed to make space for line copy");
      fclose(original);
      fclose(copy);
      return -1;
    }

    // Attempt to pull the host and port out of the line
    char *string_host = strtok(to_modify, " ");
    char *string_port = strtok(NULL, " ");

    // If strtok failed to parse, some formatting is wrong
    if (string_host == NULL || string_port == NULL) {
      fprintf(stderr, "malformed status file\n");
      fclose(original);
      fclose(copy);
      return -1;
    }

    // Convert the port into an integer value
    unsigned short file_port = atoi(string_port);

    // Do not copy across the target line, effectively deleting it
    if (file_port == port && strcmp(string_host, host) == 0) {
      continue;
    }

    // Copy any other lines from original to copy
    fputs(line, copy);
    free(to_modify);
  }

  // Save and close both files
  if (fclose(original)) {
    perror("failed to close original status file");
    fclose(copy); //< attempt to close copy, even though it could fail
    return -1;
  }
  if (fclose(copy)) {
    perror("failed to close copied status file");
    return -1;
  }

  // Delete the original and replace it with the copy
  if (unlink(path) == -1) {
    perror("failed to delete original file");
    free(temp_path);
    free(path);
    return -1;
  }
  if (rename(temp_path, path) == -1) {
    perror("failed to rename old file to new file");
    free(temp_path);
    free(path);
    return -1;
  }

  // Free the memory storing both file names
  free(temp_path);
  free(path);
  return 0;
}

void print_give_status() {
  // Locate and open the status file
  char *path = status_file_path();
  if (path == NULL) {
    perror("failed to allocate space for status file name");
    return;
  }
  FILE *stream = fopen(path, "r");
  if (stream == NULL) {
    // Ignore ENOENT, file does not exits
    if (errno != ENOENT) {
      perror("failed to open status file");
    }
    free(path);
    return;
  }

  // Read through every line of the file, printing them out
  size_t sz = 0;
  char *line = NULL;
  ssize_t chars_read;
  while ((chars_read = getline(&line, &sz, stream)) != -1) {
    // strtok for each one of the fields we're interested in
    char *host = strtok(line, " ");
    char *port = strtok(NULL, " ");
    char *file_name = strtok(NULL, " ");
    char *target_user = strtok(NULL, " ");

    // Just skip the line if anything went wrong
    if (host == NULL || port == NULL || file_name == NULL ||
        target_user == NULL) {
      continue;
    }

    // Attempt to remove the \n from the end of target_user
    char *newline = strchr(target_user, '\n');
    if (newline != NULL) {
      *newline = '\0';
    } else {
      // Malformed file
      continue;
    }

    // Pretty print out the data we just got
    printf("File: %s\n", file_name);
    printf("  to user: %s\n", target_user);
    printf("  running on: %s, port %s\n", host, port);
  }

  if (fclose(stream)) {
    perror("failed to close status file");
    free(path);
    return;
  }

  free(path);
}

#include <dirent.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "file.h"
#include "utils.h"

// Manually keep track of how many files we have open
#define MAX_FILES_OPEN 8
int files_open = 0;

void free_file(file_t *file) {
  free(file->name);
  switch (file->type) {
    case F_REG:
      // Regular files just need to have their data freed
      free(file->contents.data);
      break;
    case F_DIR:
      // For directories, we need to recursively free all the entries
      for (size_t i = 0; i < file->size; i++) {
        free_file(file->contents.entries[i]);
      }

      // Then, we can free where the entries are stored
      free(file->contents.entries);
      break;
  }
  free(file);
}

/**
 * Read a regular file into a pointer.
 *
 * \param path  Path to the file
 * \param file  Pointer to file struct. Data will be filled out.
 * \return      0 if everything went well, -1 on error
 */
int read_regular(char *path, file_t *file) {
  // try to open the file
  FILE *stream = fopen(path, "r");
  if (stream == NULL) {
    perror("failed to open regular file");
    return -1;
  }

  // seek to the end of the file, store its size, then seek back to the front
  if (fseek(stream, 0, SEEK_END) != 0) {
    perror("failed to seek to file end");
    return -1;
  }
  file->size = ftell(stream);
  if (fseek(stream, 0, SEEK_SET) != 0) {
    perror("failed to seek to file start");
    return -1;
  }

  // make space for the file contents
  file->contents.data = malloc(file->size);
  if (file->contents.data == NULL) {
    perror("failed to malloc space for file contents");
    return -1;
  }

  // read the contents of the file into the malloc'd space
  if (fread(file->contents.data, 1, file->size, stream) != file->size) {
    perror("failed to read file contents");
    return -1;
  }

  // All good!
  return 0;
}

/**
 * Read a directory into a pointer, and all the files inside recursively.
 *
 * \param path   Path to the directory
 * \param file   Struct to read the file into
 * \param nopen  Number of files currently open
 * \return       0 if everything went well, -1 on error
 */
int read_directory(char *path, file_t *file) {
  DIR *dir = opendir(path);
  if (dir == NULL) {
    perror("failed to open directory");
    return -1;
  }

  file->contents.entries = NULL;
  file->size = 0;

  struct dirent *entry;
  while ((entry = readdir(dir)) != NULL) {
    // Ignore the special files . and ..
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
      continue;
    }

    // Malloc space for the path to the next entry
    int next_path_len = strlen(path) + strlen(entry->d_name);
    char *next_path = malloc(sizeof(char) * (next_path_len + 1));
    if (next_path == NULL) {
      perror("failed to allocate space for path");
      return -1;
    }

    // Fill out the path to the next entry
    strcpy(next_path, path);
    strcat(next_path, entry->d_name);

    // Realloc the entries one larger to make space for the new file
    file->contents.entries = realloc(file->contents.entries,
        (file->size + 1) * sizeof(file_t));

    // Set that entry by recursively calling read_file on it
    file_t *entry_file = read_file(next_path);
    if (entry_file == NULL) {
      return -1;
    }
    file->contents.entries[file->size] = entry_file;

    // Free the malloc'd path
    free(next_path);

    // Increase the stored number of entries
    file->size++;
  }

  // Close the directory now
  if (closedir(dir) == -1) {
    perror("failed to close directory");
    return -1;
  }

  // All went well!
  return 0;
}

file_t *read_file(char *path) {
  // check that we don't have too many files open
  if (files_open > MAX_FILES_OPEN) {
    fprintf(stderr, "Exceeded max of %d files open at once due to directory recursion!\n", MAX_FILES_OPEN);
    return NULL;
  }

  // stat the file, also checking that it exists
  struct stat st;
  if (stat(path, &st) == -1) {
    perror("could not stat file");
    return NULL;
  }

  // Create space to store file info
  file_t *new = malloc(sizeof(file_t));
  if (new == NULL) {
    perror("could not allocate file struct");
    // free nothing, no allocations have been made
    return NULL;
  }

  // Store the name, trimming off the start of the path
  new->name = strdup(get_shortname(path));
  if (new->name == NULL) {
    perror("could not allocate file name");
    free_file(new);
    return NULL;
  }

  // Handle the file contents. May be a directory or a regular file
  if (S_ISREG(st.st_mode)) {
    new->type = F_REG;
    new->contents.data = NULL;

    // Attempt to read the file contents and return them.
    files_open++;
    if (read_regular(path, new) == -1) {
      free_file(new);
      return NULL;
    }
    files_open--;
    return new;
  } else if (S_ISDIR(st.st_mode)) {
    new->type = F_DIR;
    new->contents.entries = NULL;

    // The path used may differ from the one provided because user
    // input can be ambiguous.
    char *actual_path = strdup(path);
    if (actual_path == NULL) {
      perror("failed to copy file path");
      return NULL;
    }

    // directory paths can end in /, or not.
    // to make recursion easier, always add it if it does not exist.
    int len = strlen(path) + 1;
    if (path[len-2] != '/') {
      actual_path = realloc(actual_path, len + 1);
      strcat(actual_path, "/");
    }

    // Attempt to recursively read the directory contents and return them
    files_open++;
    if (read_directory(actual_path, new) == -1) {
      free_file(new);
      return NULL;
    }
    files_open--;

    // Free duplicated string
    free(actual_path);

    return new;
  } else {
    fprintf(stderr, "file %s is neither directory nor regular\n", path);
    free(new);
    return NULL;
  }
}

/**
 * Write a regular file to a spot on disk. Writes to the path specified by
 * strcat(path, file->name).
 *
 * \param path  Path to write the file to.
 * \param file  File data to write.
 * \return      0 if everything went well, -1 on error
 */
int write_regular(char *path, file_t *file) {
  // Construct the path to the file
  int file_path_len = strlen(path) + strlen(file->name);
  char *file_path = malloc(sizeof(char) * (file_path_len + 1));
  if (file_path == NULL) {
    perror("failed to allocate space for filename");
    return -1;
  }
  strcpy(file_path, path);
  strcat(file_path, file->name);

  // If something exists there, something went wrong
  if (access(file_path, F_OK) == 0) {
    fprintf(stderr, "refusing to overwrite existing file %s\n", file_path);
    free(file_path);
    return -1;
  }

  // Open that file for writing
  FILE *stream = fopen(file_path, "w");
  if (stream == NULL) {
    perror("failed to open file");
    free(file_path);
    return -1;
  }

  // Write the data into that file
  if (fwrite(file->contents.data, 1, file->size, stream) != file->size) {
    perror("failed to write file contents");
    free(file_path);
    return -1;
  }

  // Close the file
  if (fclose(stream)) {
    perror("failed to close file");
    free(file_path);
    return -1;
  }

  // All went well!
  free(file_path);
  return 0;
}

/**
 * Write a directory to a spot on disk, and all the files inside of it
 * recursively.
 *
 * \param path  Path to write the directory to.
 * \param file  File data to write.
 * \return      0 if everything went well, -1 on error
 */
int write_directory(char *path, file_t *file) {
  // Construct the path to the directory
  int dir_path_len = strlen(path) + strlen(file->name) + strlen("/");
  char *dir_path = malloc(sizeof(char) * (dir_path_len + 1));
  if (dir_path == NULL) {
    perror("failed to allocate space for filename");
    return -1;
  }
  strcpy(dir_path, path);
  strcat(dir_path, file->name);
  strcat(dir_path, "/");

  if (access(dir_path, F_OK) == 0) {
    fprintf(stderr, "refusing to overwrite existing directory %s\n", dir_path);
    free(dir_path);
    return -1;
  }

  // Attempt to create that directory
  // mode 0777 means we leave dir permissions up to the user's umask
  if (mkdir(dir_path, 0777) == -1) {
    perror("failed to create directory");
    free(dir_path);
    return -1;
  }

  // For all the directory entries, attempt to write them as well
  for (int i = 0; i < file->size; i++) {
    int rc = write_file(dir_path, file->contents.entries[i]);
    if (rc == -1) {
      free(dir_path);
      return -1;
    }
  }

  // All done!
  free(dir_path);
  return 0;
}

int write_file(char *path, file_t *file) {
  switch (file->type) {
    case F_REG:
      if (write_regular(path, file) == -1) {
        return -1;
      }
      break;
    case F_DIR:
      if (write_directory(path, file) == -1) {
        return -1;
      }
      break;
  }

  return 0;
}

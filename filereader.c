#include "filereader.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "utils.h"

// Refuse to store more than 256MB of file data
#define MAX_FILE_STORAGE 0x10000000
size_t file_storage_used = 0;

void free_file(file_t* file) {
  if (file == NULL) {
    return;
  }
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
int read_regular(char* path, file_t* file) {
  // try to open the file
  FILE* stream = fopen(path, "r");
  if (stream == NULL) {
    perror("Failed to open regular file");
    return -1;
  }

  // try to stat the file, so we can get its size and mode
  struct stat st;
  if (fstat(fileno(stream), &st) == -1) {
    perror("Failed to stat file");
    return -1;
  }
  file->size = st.st_size;
  file->mode = st.st_mode;

  // Check whether storing this file puts us over self-set limit
  if (file_storage_used + file->size > MAX_FILE_STORAGE) {
    fprintf(stderr, "File storage would exceed max of 256MB. Refusing to continue.\n");
    return -1;
  }

  // Allocate space, and check that it got allocated okay
  file->contents.data = malloc(file->size);
  file_storage_used += file->size;
  if (file->contents.data == NULL) {
    perror("Failed to malloc space for file contents");
    return -1;
  }

  // read the contents of the file into the malloc'd space
  if (fread(file->contents.data, 1, file->size, stream) != file->size) {
    perror("Failed to read file contents");
    free(file->contents.data);
    return -1;
  }

  // Close the file
  if (fclose(stream)) {
    perror("Failed to close regular file");
    free(file->contents.data);
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
 * \return       0 if everything went well, -1 on error
 */
int read_directory(char* path, file_t* file) {
  DIR* dir = opendir(path);
  if (dir == NULL) {
    perror("Failed to open directory");
    return -1;
  }

  // Set the contents and size to defaults
  file->contents.entries = NULL;
  file->size = 0;

  // First, stat the directory so we can get its mode
  struct stat st;
  if (stat(path, &st) == -1) {
    perror("Failed to stat directory");
    return -1;
  }
  file->mode = st.st_mode;

  // Prepare to store the paths of all entries
  char** all_entries = NULL;
  int num_entries = 0;

  // Read all directory entries (except . and ..)
  struct dirent* entry;
  while ((entry = readdir(dir)) != NULL) {
    // Ignore the special files . and ..
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
      continue;
    }

    // TODO: should we ignore dotfiles? if (entry->d_name[0] == '.')?

    // Malloc space for the path to the next entry
    int next_path_len = strlen(path) + strlen(entry->d_name);
    char* next_path = malloc(sizeof(char) * (next_path_len + 1));
    if (next_path == NULL) {
      perror("Failed to allocate space for path");
      return -1;
    }

    // Fill out the path to the next entry
    strcpy(next_path, path);
    strcat(next_path, entry->d_name);

    // Realloc the list of entries one larger to make space
    all_entries = realloc(all_entries, (num_entries + 1) * sizeof(char*));

    // Store that path to be recursed into later
    all_entries[num_entries] = next_path;
    num_entries++;
  }
  file->size = num_entries;

  file->contents.entries = calloc(file->size, sizeof(file_t));

  // Close the directory now
  if (closedir(dir) == -1) {
    perror("Failed to close directory");
    return -1;
  }

  // Recurse into each entry, storing the data
  for (int i = 0; i < num_entries; i++) {
    // Make space for a new file
    file_t* entry_file = malloc(sizeof(file_t));
    if (entry_file == NULL) {
      // Free everything else
      for (int j = i; j < num_entries; j++) {
        free(all_entries[j]);
      }
      free(all_entries);
      return -1;
    }

    // Read the entry
    if (read_file(all_entries[i], entry_file) == -1) {
      // Free everything else
      for (int j = i; j < num_entries; j++) {
        free(all_entries[j]);
      }
      free(all_entries);
      return -1;
    }
    file->contents.entries[i] = entry_file;

    // Also free the path to that entry, we're done w/ it
    free(all_entries[i]);
  }

  // Free the temp memory we used to store the entries
  free(all_entries);

  // All went well!
  return 0;
}

// TODO redocument in .h
int read_file(char* path, file_t* file) {
  // Store the name, trimming off the start of the path
  file->name = strdup(get_shortname(path));
  if (file->name == NULL) {
    perror("Failed to allocate file name");
    return -1;
  }

  // stat the file, also checking that it exists
  struct stat st;
  if (stat(path, &st) == -1) {
    perror("Failed to stat file");
    return -1;
  }

  // Handle the file contents. May be a directory or a regular file
  else if (S_ISREG(st.st_mode)) {
    file->type = F_REG;
    file->contents.data = NULL;

    // Attempt to read the file contents and return them.
    if (read_regular(path, file) == -1) {
      return -1;
    }
    return 0;
  } else if (S_ISDIR(st.st_mode)) {
    file->type = F_DIR;
    file->contents.entries = NULL;

    // The path used may differ from the one provided because user
    // input can be ambiguous.
    char* actual_path = strdup(path);
    if (actual_path == NULL) {
      perror("Failed to copy file path");
      return -1;
    }

    // directory paths can end in /, or not.
    // to make recursion easier, always add it if it does not exist.
    int len = strlen(path) + 1;
    if (path[len - 2] != '/') {
      actual_path = realloc(actual_path, len + 1);
      strcat(actual_path, "/");
    }

    // Attempt to recursively read the directory contents and return them
    if (read_directory(actual_path, file) == -1) {
      return -1;
    }

    // Free duplicated string
    free(actual_path);

    return 0;
  } else {
    fprintf(stderr, "File %s is neither a regular file nor a directory!\n", path);
    return -1;
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
int write_regular(char* path, file_t* file) {
  // Construct the path to the file
  int file_path_len = strlen(path) + strlen(file->name);
  char* file_path = malloc(sizeof(char) * (file_path_len + 1));
  if (file_path == NULL) {
    perror("Failed to allocate space for filename");
    return -1;
  }
  strcpy(file_path, path);
  strcat(file_path, file->name);

  // If something exists there, something went wrong
  if (access(file_path, F_OK) == 0) {
    fprintf(stderr, "Refusing to overwrite existing file %s\n", file_path);
    free(file_path);
    return -1;
  }

  // Open that file for writing
  int fd = open(file_path, O_WRONLY | O_CREAT, file->mode);
  if (fd == -1) {
    perror("Failed to open file");
    free(file_path);
    return -1;
  }

  // Write the data into that file
  if (write(fd, file->contents.data, file->size) != file->size) {
    perror("Failed to write file contents");
    free(file_path);
    return -1;
  }

  // Close the file
  if (close(fd)) {
    perror("Failed to close file");
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
int write_directory(char* path, file_t* file) {
  // Construct the path to the directory
  int dir_path_len = strlen(path) + strlen(file->name) + strlen("/");
  char* dir_path = malloc(sizeof(char) * (dir_path_len + 1));
  if (dir_path == NULL) {
    perror("Failed to allocate space for filename");
    return -1;
  }
  strcpy(dir_path, path);
  strcat(dir_path, file->name);
  strcat(dir_path, "/");

  if (access(dir_path, F_OK) == 0) {
    fprintf(stderr, "Fefusing to overwrite existing directory %s\n", dir_path);
    free(dir_path);
    return -1;
  }

  // Attempt to create that directory
  if (mkdir(dir_path, file->mode) == -1) {
    perror("Failed to create directory");
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

int write_file(char* path, file_t* file) {
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

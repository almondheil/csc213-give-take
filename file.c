// from myls.c
// todo: remove unneeded .h files
#include <dirent.h>
#include <errno.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

// todo: do i need this? for uint8_t
#include <stdint.h>

#include "utils.h"
// ^ for get_shortname(path)

typedef struct file {
  // a file can be normal (containing bytes) or directory (containing more files)
  enum { F_REG, F_DIR } type;

  // both normal and directory have a name and size
  char *name;
  size_t size;

  union {
    // f_reg only
    // size is interpreted as the size of file_data in bytes
    uint8_t *data;

    // f_dir only
    // size is interpreted as the number of directory entries
    struct file **entries;
  } contents;
} file_t;

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

file_t *read_path(char *path);

int read_regular(char *path, file_t *file) {
  printf("LOG: reading regular file %s\n", path);

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

int read_directory(char *path, file_t *file) {
  printf("LOG: reading directory %s\n", path);

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
    char *next_path = malloc(next_path_len + 1);
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

    // Set that entry by recursively calling read_path on it
    file->contents.entries[file->size] = read_path(next_path);

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

file_t *read_path(char *path) {
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
  // TODO: should get_shortname go in here? depends if anywhere else uses it
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
    if (read_regular(path, new) == -1) {
      free_file(new);
      return NULL;
    }
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
    if (read_directory(actual_path, new) == -1) {
      free_file(new);
      return NULL;
    }

    // Free duplicated string
    free(actual_path);

    return new;
  } else {
    fprintf(stderr, "file %s is neither directory nor regular\n", path);
    free(new);
    return NULL;
  }
}

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "gimme a filename\n");
    return 1;
  }

  file_t *f = read_path(argv[1]);
  free_file(f);

  printf("I mean it didn't crash\n");
  return 0;
}

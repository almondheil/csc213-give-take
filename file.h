#include <stdint.h>

// File structure. Can either be a regular file or a directory.
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

void free_file(file_t *file);

int read_regular(char *path, file_t *file);

int read_directory(char *path, file_t *file);

file_t *read_file(char *path);

int write_regular(char *path, file_t *file);

int write_directory(char *path, file_t *file);

int write_file(char *path, file_t *file);

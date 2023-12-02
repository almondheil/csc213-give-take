#pragma once

#include <stdint.h>

typedef enum {
  F_REG, //< regular file
  F_DIR  //< directory
} filetype;

// File structure. Can either be a regular file or a directory.
typedef struct file {
  filetype type;

  // both normal and directory have a name and size
  char *name;
  size_t size;

  // Union. Holds either data pointer or pointer to entries.
  union {
    uint8_t *data;         //< F_REG only
    struct file **entries; //< F_DIR only
  } contents;
} file_t;

/**
 * Free an allocated file of unknown type recursively.
 *
 * \param file  File to free.
 */
void free_file(file_t *file);

/**
 * Read a regular file into a pointer.
 *
 * \param path  Path to the file
 * \param file  Pointer to file struct. Data will be filled out.
 * \return      0 if everything went well, -1 on error
 */
int read_regular(char *path, file_t *file);

/**
 * Read a directory into a pointer, and all the files inside recursively.
 *
 * \param path  Path to the directory
 * \param file  Struct to read the file into
 * \return      0 if everything went well, -1 on error
 */
int read_directory(char *path, file_t *file);

/**
 * Read a file of unknown type, returning malloc'd memory containing the file info.
 *
 * \param path  Path to the file.
 * \return      Pointer to malloc'd file.
 */
file_t *read_file(char *path);

/**
 * Write a regular file to a spot on disk. Writes to the path specified by
 * strcat(path, file->name).
 *
 * \param path  Path to write the file to.
 * \param file  File data to write.
 * \return      0 if everything went well, -1 on error
 */
int write_regular(char *path, file_t *file);

/**
 * Write a directory to a spot on disk, and all the files inside of it
 * recursively.
 *
 * \param path  Path to write the directory to.
 * \param file  File data to write.
 * \return      0 if everything went well, -1 on error
 */
int write_directory(char *path, file_t *file);

/**
 * Write a file of unknown type to disk.
 *
 * \param path  Path to write the file to.
 * \param file  File data to write.
 * \return      0 if everything went well, -1 on error
 */
int write_file(char *path, file_t *file);

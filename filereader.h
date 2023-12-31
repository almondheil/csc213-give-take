/**
 * filereader.h
 *
 * Read and write files on disk, in a form that can be sent across the network.
 */

#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>

typedef enum {
  F_REG,  //< regular file
  F_DIR   //< directory
} filetype;

// File structure. Can either be a regular file or a directory.
typedef struct file {
  filetype type;

  // both regular and directory have these
  char* name;
  size_t size;
  mode_t mode;

  // Holds either data pointer or pointer to entries.
  union {
    uint8_t* data;          //< F_REG only
    struct file** entries;  //< F_DIR only
  } contents;
} file_t;

/**
 * Free an allocated file of unknown type recursively.
 *
 * \param file  File to free.
 */
void free_file(file_t* file);

/**
 * Read a file of unknown type, returning malloc'd memory containing the file
 * info. For directories, this includes any directory entries.
 *
 * \param path  Path to the file.
 * \param file  Pointer to read file into.
 * \return      0 on success, -1 on error
 */
int read_file(char* path, file_t* file);

/**
 * Write a file of unknown type to disk.
 *
 * \param path  Path to write the file to.
 * \param file  File data to write.
 * \return      0 if everything went well, -1 on error
 */
int write_file(char* path, file_t* file);

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "communication.h"

int read_file_contents(filedata_t * file_data, FILE* stream) {
	// Seek to the end of the file so we can get its size
	if (fseek(stream, 0, SEEK_END) != 0) {
		perror("Could not seek to end of file");
		return -1;
	}

	// Find the size
	file_data->size = ftell(stream);

	// Seek back to the start so we can read it
	if (fseek(stream, 0, SEEK_SET) != 0) {
		perror("Could not seek to start of file");
		return -1;
	}

	// Allocate space, and make sure everything fits
	file_data->data = malloc(file_data->size);
	if (file_data->data == NULL) {
		perror("Could not allocate space to store file contents");
		return -1;
	}

	// Read the file data into the malloced space
	if (fread(file_data->data, 1, file_data->size, stream) != file_data->size) {
		fprintf(stderr, "Failed to read the entire file\n");
		return -1;
	}

	return 0;
}

int send_file(int fifo_fd, filedata_t* file_data) {
	// Send how long the data is
	size_t name_len = sizeof(char) * strlen(file_data->name);
	if (write(fifo_fd, &name_len, sizeof(size_t)) != sizeof(size_t)) {
		return -1;
	}

	// Send how many bytes the file is
	size_t data_size = file_data->size;
	if (write(fifo_fd, &data_size, sizeof(size_t)) != sizeof(size_t)) {
		return -1;
	}

	// Send the name of the file
	size_t bytes_written = 0;
	while (bytes_written < name_len) {
		// write the remaining message
		ssize_t rc = write(fifo_fd, file_data->name + bytes_written, name_len - bytes_written);

		// if the write failed, there was an error
		if (rc <= 0) return -1;

		bytes_written += rc;
	}

	// Send the data of the file
	bytes_written = 0;
	while (bytes_written < data_size) {
		// write the remaining message
		ssize_t rc = write(fifo_fd, file_data->data + bytes_written, data_size - bytes_written);

		// if the write failed, there was an error
		if (rc <= 0) return -1;

		bytes_written += rc;
	}

	return 0;
}

filedata_t* recv_file(int fifo_fd) {
	// Read the size of the filename
	size_t filename_len;
	if (read(fifo_fd, &filename_len, sizeof(size_t)) != sizeof(size_t)) {
		return NULL;
	}

	// Read the size of the file
	size_t data_size;
	if (read(fifo_fd, &data_size, sizeof(size_t)) != sizeof(size_t)) {
		return NULL;
	}

	// Create a data
	filedata_t* data = malloc(sizeof(filedata_t));
	data->size = data_size;

	// Read the filename
	data->name = malloc(filename_len + 1);
	size_t bytes_read = 0;
	while (bytes_read < filename_len) {
		ssize_t rc = read(fifo_fd, data->name + bytes_read, filename_len - bytes_read);

		if (rc <= 0) {
			free(data->name);
			free(data);
			return NULL;
		}

		bytes_read += rc;
	}
	data->name[filename_len] = '\0';

	// // TODO: Read the file data
	// data->data = malloc(data_size);
	// size_t bytes_read = 0;
	// while (bytes_read < data_size) {
	// 	ssize_t rc = read(fifo_fd, data->data + bytes_read, data_size - bytes_read);

	// 	if (rc <= 0) {
	// 		free(data->data);
	// 		free(data->name);
	// 		free(data);
	// 		return NULL;
	// 	}

	// 	bytes_read += rc;
	// }

	return data;
}

char * find_fifo_name(char* give_username, char* take_username) {
	// Malloc space for the name we want to use
	int fifo_name_len = strlen("/home/") + strlen(give_username) +
		strlen("/.give-take-") + strlen(take_username) + 1;
	char * fifo_name = malloc(sizeof(char) * fifo_name_len);

	// Copy all the parts into the string
	strcpy(fifo_name, "/home/");
	strcat(fifo_name, give_username);
	strcat(fifo_name, "/.give-take-");
	strcat(fifo_name, take_username);
	return fifo_name;
}

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "message.h"

int send_file(int sock_fd, file_t* file_data) {
	// Send how long the data is
	size_t name_len = sizeof(char) * strlen(file_data->name);
	if (write(sock_fd, &name_len, sizeof(size_t)) != sizeof(size_t)) {
		return -1;
	}

	// Send how many bytes the file is
	size_t data_size = file_data->size;
	if (write(sock_fd, &data_size, sizeof(size_t)) != sizeof(size_t)) {
		return -1;
	}

	// Send the name of the file
	size_t bytes_written = 0;
	while (bytes_written < name_len) {
		// write the remaining message
		ssize_t rc = write(sock_fd, file_data->name + bytes_written, name_len - bytes_written);

		// if the write failed, there was an error
		if (rc <= 0) return -1;

		bytes_written += rc;
	}

	// Send the data of the file
	bytes_written = 0;
	while (bytes_written < data_size) {
		// write the remaining message
		ssize_t rc = write(sock_fd, file_data->data + bytes_written, data_size - bytes_written);

		// if the write failed, there was an error
		if (rc <= 0) return -1;

		bytes_written += rc;
	}

	return 0;
}

file_t* recv_file(int sock_fd) {
	// Read the size of the filename
	size_t filename_len;
	if (read(sock_fd, &filename_len, sizeof(size_t)) != sizeof(size_t)) {
		return NULL;
	}

	// Read the size of the file
	size_t data_size;
	if (read(sock_fd, &data_size, sizeof(size_t)) != sizeof(size_t)) {
		return NULL;
	}

	// Create a data
	file_t* data = malloc(sizeof(file_t));
	data->size = data_size;

	// Read the filename
	data->name = malloc(filename_len + 1);
	size_t bytes_read = 0;
	while (bytes_read < filename_len) {
		ssize_t rc = read(sock_fd, data->name + bytes_read, filename_len - bytes_read);

		if (rc <= 0) {
			free(data->name);
			free(data);
			return NULL;
		}

		bytes_read += rc;
	}

	// Null terminate the filename
	data->name[filename_len] = '\0';

	// Read the data of the file
	data->data = malloc(data_size);
	bytes_read = 0;
	while (bytes_read < data_size) {
		ssize_t rc = read(sock_fd, data->data + bytes_read, data_size - bytes_read);

		if (rc <= 0) {
			free(data->data);
			free(data->name);
			free(data);
			return NULL;
		}

		bytes_read += rc;
	}

	// Return the filled out struct
	return data;
}

int send_request(int sock_fd, request_t* req) {
	// Send how long the name is
	size_t name_len = sizeof(char) * strlen(req->name);
	if (write(sock_fd, &name_len, sizeof(size_t)) != sizeof(size_t)) {
		return -1;
	}

	// Send the name over
	size_t bytes_written = 0;
	while (bytes_written < name_len) {
		// write the remaining message
		ssize_t rc = write(sock_fd, req->name + bytes_written, name_len - bytes_written);

		// if the write failed, there was an error
		if (rc <= 0) return -1;

		bytes_written += rc;
	}

	// Send the request value
	if (write(sock_fd, &req->action, sizeof(action_t)) != sizeof(action_t)) {
		return -1;
	}

	return 0;
}

request_t* recv_request(int sock_fd) {
	// Read the length of the name
	size_t name_len;
	if (read(sock_fd, &name_len, sizeof(size_t)) != sizeof(size_t)) {
		return NULL;
	}

	// Create a struct to store the values we'll receive
	request_t* req = malloc(sizeof(request_t));
	req->name = malloc(name_len + 1);

	// Read the name
	size_t bytes_read = 0;
	while (bytes_read < name_len) {
		ssize_t rc = read(sock_fd, req->name + bytes_read, name_len - bytes_read);

		if (rc <= 0) {
			free(req->name);
			free(req);
			return NULL;
		}

		bytes_read += rc;
	}

	// Null terminate the name
	req->name[name_len] = '\0';

	// Read the requested action
	if (read(sock_fd, &req->action, sizeof(action_t)) != sizeof(action_t)) {
		free(req->name);
		free(req);
		return NULL;
	}

	// Return the request now that we've read all its data
	return req;
}

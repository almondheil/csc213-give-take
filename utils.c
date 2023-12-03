#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "utils.h"

char *get_shortname(char *path) {
  // Find the last / character in the path (if it exists)
  char *last_slash = strrchr(path, '/');

  // If there is a /, trim to only everything after.
  // this turns /path/to/file.ext into file.ext
  if (last_slash != NULL) {
    return last_slash + 1;
  } else {
    return path;
  }
}

void parse_connection_info(char *in, char *hostname, unsigned short *port) {
  // Determine if there is a : in the input string

  // Hold up to two substrings - to the hostname, and to the port
  char *parts[2];

  // Read up to two substrings into parts
  char *token;
  char *rest = in;
  int i = 0;
  while ((token = strtok_r(rest, ":", &rest))) {
    if (i >= 2) {
      break;
    }

    parts[i] = token;
    i++;
  }

  // Determine if input was "host:port" or "port"
  if (i == 1) {
    // Only a port was provided, parts[0] contains it
    strcpy(hostname, "localhost");
    *port = atoi(parts[0]);
  } else {
    // Both a port and a hostname were provided
    strcpy(hostname, parts[0]);
    strcat(hostname, ".cs.grinnell.edu");
    *port = atoi(parts[1]);
  }
}

char *get_username() {
  // Get the effective uid
  uid_t uid = geteuid();

  // Find the user associated with that uid
  struct passwd *pw = getpwuid(uid);
  if (pw == NULL) {
    return NULL;
  }

  return pw->pw_name;
}

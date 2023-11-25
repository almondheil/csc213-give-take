#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BASE 36
#define CODE_LEN 4

// Conversion table
const char conversion[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
  'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'};

void port2alpha(unsigned short port, char *code) {
  // Determine the code chars
  int place = 0;
  while (port > 0) {
    code[CODE_LEN - place - 1] = conversion[port % BASE];
    port /= BASE;
    place++;
  }

  // Null-terminate the string
  code[place] = '\0';
}

void alpha2port(const char *code, unsigned short *port) {
  int val = 0;
  for (int i = 0; i < strlen(code); i++) {
    val *= BASE;
    printf("multiplied by BASE\n");

    // Find code[i] within conversion table
    for (int j = 0; j < BASE; j++) {
      if (code[i] == conversion[j]) {
        val += i;
        printf("added a value of %d\n", j);
        break;
      }
    }
  }
  printf("val = %d\n", val);

  *port = val;
}

int main(int argc, char * argv[]) {
  unsigned short port = atoi(argv[1]);
  printf("original port = %d\n", port);

  char code[100];
  port2alpha(port, code);
  printf("converted code = %s\n", code);

  unsigned short reverted = 0;
  alpha2port(code, &reverted);
  printf("and back again = %d\n", reverted);

}

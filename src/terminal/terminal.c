
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "bp_utils.h"

#define MAX_STRLEN 128

int main(int arc, char** argv) {

  printf("Hello! Welcome to BlackParrot Terminal Demo!\n");
  printf("Enter a string and we will print it back.\n");
  printf("Type 'quit' to exit\n");
  printf("(This terminal does not buffer so, original string will not print while typing)\n");

  char input[MAX_STRLEN];

  while (1) {
    int i = 0;

    fflush(stdin);
    printf("fgets: >> ");
    fflush(stdout);
    fgets(input, MAX_STRLEN-1, stdin);
    printf("%s\n", input);

    printf("gets: >> ");
    fflush(stdout);
    gets(input);
    printf("%s\n", input);

    printf("scanf: >> ");
    fflush(stdout);
    scanf("%s", input);
    printf("%s\n", input);

    if (!strncmp("quit", input, 4)) {
      return 0;
    }
  }
}


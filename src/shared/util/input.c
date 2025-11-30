#include <stdbool.h>
#include <stdio.h>

#include "util/input.h"

#include <string.h>

void getStringInput(char *inputName, char *inputStr, const int strLength) {
  bool inputSuccess = false;
  while (!inputSuccess) {
    printf("Please enter your %s:\n", inputName);

    if (!fgets(inputStr, strLength, stdin)) {
      printf("Failed to read user input. Please try again:\n");
    } else {
      inputSuccess = true;
      inputStr[strcspn(inputStr, "\n")] = '\0';
    }
  }
}

/**
 * Generic method for retrieving a long integer from keyboard input.
 * @param inputName The name of the input you're prompting for
 * @return The long value input by the user
 */
unsigned long getLongInput(char *inputName) {
  long input = -1;
  while (input < 0) {
    printf("Please enter your %s:\n", inputName);
    char line[64];

    if (fgets(line, sizeof(line), stdin)) {
      sscanf(line, "%ld", &input);
      if (sscanf(line, "%d", &input) != 1 || input < 0) {
        printf("Invalid %s entered. Please try again!\n", inputName);
      }
    } else {
      printf("Failed to read user input. Please try again:\n");
    }
  }

  return (unsigned long) input;
}

int getInt() {

  int selected = 0;
  char line[64];

  if (fgets(line, sizeof(line), stdin)) {
    sscanf(line, "%d", &selected);
  }
  return selected;
}
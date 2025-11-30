#ifndef COSC522_LODI_INPUT_H
#define COSC522_LODI_INPUT_H

int getInt();

void getStringInput(char *inputName, char *inputStr, int strLength);

/**
 * Generic method for retrieving a long integer from keyboard input.
 * @param inputName The name of the input you're prompting for
 * @return The long value input by the user
 */
unsigned long getLongInput(char *inputName);


#endif

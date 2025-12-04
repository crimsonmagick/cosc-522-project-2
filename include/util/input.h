#ifndef COSC522_LODI_INPUT_H
#define COSC522_LODI_INPUT_H
/**
*  Interface for retrieving user input.
 */

/**
 * Gets an int from user input
 * @return user input int
 */
int getInt();

/**
 * Gets a string from input
 *
 * @param inputName Input name to prompt for
 * @param inputStr Output string (pointer)
 * @param strLength Length of output string
 */
void getStringInput(char *inputName, char *inputStr, int strLength);

/**
 * Generic method for retrieving a long integer from keyboard input.
 * @param inputName The name of the input you're prompting for
 * @return The long value input by the user
 */
unsigned long getLongInput(char *inputName);


#endif

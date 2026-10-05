// Practice #3: The String Reverser (Strings & Pointers)
// In C, strings aren't a special type; they are just arrays of characters ending with a null terminator (\0). Let's play with them.

// The Mission: Write a program that takes a word from the user and prints it backward.

// Requirements:

// Declare a character array (string) with a size of 50.

// Input: Use scanf("%s", your_variable) to get a single word.

// Length: Use strlen() (you'll need #include <string.h>) to find out how long the word is.

// Reverse: Use a for loop to print the characters starting from the end of the string moving toward the front.

#include <stdio.h>
#include <string.h>

int main(void) {
	int len = 50;
	char string_chars[len];
	char reversed[len];

	printf("Input string: \n");
	scanf("%49s", &string_chars);

	int strlenn = strlen(string_chars);

	for (int i = 0; i < strlenn; ++i) {
		reversed[strlenn - i - 1] = string_chars[i];
	}

	reversed[strlenn] = '\0';

	printf("Reversed:%s ", reversed);
	return 0;
}
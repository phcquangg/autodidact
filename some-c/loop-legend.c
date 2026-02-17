// The Mission: Write a program that finds the largest number in an array.

// Requirements:
// Create an integer array of size 5.
// Use a for loop to ask the user to input 5 integers to fill the array.
// Write a second loop to iterate through the array and find the maximum value.
// Print the maximum value to the console.

#include <stdio.h>

int getInput(int i) {
	int n;
	printf("Input number #%d\n", i);
	scanf("%d", &n);

	if (n) {
		return n;
	} else {
		return 0;
	}
}

int main(void) {
	int list_size = 5;
	int list[list_size];

	for (int i = 0; i < list_size; i++) {
		list[i] = getInput(i + 1);
	}

	int max = list[0];

	for (int i = 0; i < list_size; ++i) {
		if (list[i] > max) {
			max = list[i];
		}
	}

	printf("The greatest number is the list is: %d", max);
	return max;
}
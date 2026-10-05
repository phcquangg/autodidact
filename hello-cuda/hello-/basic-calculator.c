#include <stdio.h>

float doMath(float a, float b, char op) {
	switch (op) {
		case '+':
		return a + b;
		case '-':
		return a - b;
		case '*':
		return a * b;
		case '/':
			if (b == 0) {
				printf("Error!!");
			} else {
		return a / b;
			}
		default:
		return 0;
	}
}

float main (void) {
	float a;
	float b;
	char o;

	printf("Input first number: ");
	scanf("%f", &a);
	printf("Input second number: ");
	scanf("%f", &b);
	printf("Input operator: ");
	scanf(" %c", &o);

	float result = doMath(a, b, o);
	printf("%.2f", result);
	return result;
}
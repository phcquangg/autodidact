// Write a function that returns a local variable by value vs by pointer. Try returning a pointer to a local variable — observe what happens. Research: stack frames, undefined behavior, why JS doesn't have this problem.
#include <stdio.h>
#include <stdlib.h>

int returnValue() {
    int num = 12;

    return num;
}

int returnPointer() {
    int num = 12;

    return num;
}

int* returnHeap() {
	int* num = malloc(sizeof(int));
	*num = 12;
	return num;	
}


int main() {
    printf("%p", returnHeap());
    return 0;
}

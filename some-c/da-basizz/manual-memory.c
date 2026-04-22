// Write a program that declares an int, a float, a char, and a pointer to each. Print their values AND their memory addresses. Research: sizeof(), format specifiers (%d, %f, %c, %p), stack vs heap.

#include <stdio.h>

int main() {
    int i = 12;
    char* c = "hello";
    float f = 1.12;

    int *pI = &i;
    char* *pC = &c;
    float *pf = &f;
    
    printf("Int: %d, %p, %zu\n", i, (void *)pI, sizeof(i));
    printf("Float: %.3f, %p, %zu\n", f, (void *)pf, sizeof(f));
    printf("Character: %s, %p, %zu\n", c, (void *)pC, sizeof(c));
    
    return 0;
}
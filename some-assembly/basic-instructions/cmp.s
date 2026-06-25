// https://developer.arm.com/documentation/ddi0406/cb/System-Level-Architecture/The-System-Level-Programmers--Model/ARM-processor-modes-and-ARM-core-registers/Program-Status-Registers--PSRs-?lang=en

.global _start

_start:
    mov r0, #4
    mov r1, #3

    // r0 - r1
    // r0 > r1 ? + : -
    // r0 == r1 -> 0
    
    cmp r0, r1
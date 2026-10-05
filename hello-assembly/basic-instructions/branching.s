// https://developer.arm.com/documentation/ddi0406/cb/Application-Level-Architecture/Instruction-Details/Conditional-execution?lang=en

.global _start

_start:
    mov r0, #4
    mov r1, #3

    cmp r0, r1
    beq c1
    b c2

c1:
    mov r2, #1

c2:
    mov r2, #2
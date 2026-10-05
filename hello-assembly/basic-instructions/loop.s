.global _start

_start:
    mov r0, #0

loop:
    cmp r0, #5
    bge end_c
    add r0, #1
    b loop

end_c:
    mov r1, #5
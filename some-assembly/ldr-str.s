.global _start

_start:
	ldr r0, =var1
	ldr r1, [r0]
	mov r2, #4

	subs r1, r1, r2

	mov r7, #1
	mov r0, r1
	svc 0

_data:
	var1: .word 15
	var2: .word 6

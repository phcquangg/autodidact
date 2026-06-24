.global _start

_start:
	ldr r0, =var1
	ldr r1, [r0]
	ldr r0, =var2
	ldr r2, [r0]

	subs r3, r1, r2
	str r3, [r0] 
	ldr r4, [r0]

_data:
	var1: .word 3
	var2: .word 1

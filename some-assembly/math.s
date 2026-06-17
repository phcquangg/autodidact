.global _start
_start:
	mov r0, #3
	mov r1, #0

	// sub r2, r0, r1
	subs r2, r1, r0

// using __s instructions to update cpsr

.global _start

_start:
  ldr r0, =a
  ldr r1, [r0]
  ldr r0, =b
  ldr r2, [r0]

  ands r1, r2, #12
  orr r1, r2, #12
  mvn r1, r2
  
_data:
  a: .word 1
  b: .word 3  
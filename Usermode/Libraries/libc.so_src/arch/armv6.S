@ 
@ Acess2 C Library
@ - By John Hodge (thePowersGang)
@ 
@ arch/armv6.S
@ - ARMv6 specific code
.globl setjmp
setjmp:
	@ RO: Buffer
	stm r0, {r0-r14}
	eor r0, r0
	mov pc, lr

.globl longjmp
longjmp:
	@ R0: Buffer
	@ R1: Value
	add r0, #8
	ldm r0, {r2-r14}
	mov r0, r1
	tst r0, r0
	addeq r0, #1
	mov pc, lr	@ Will return to after setjmp


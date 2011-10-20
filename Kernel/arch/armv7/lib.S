/*
 * Acess2 ARM
 * - By John Hodge (thePowersGang)
 *
 * arch/arm7/lib.S
 * - Assembly editions of library functions
 */
#include "include/assembly.h"

.globl __memcpy_byte
__memcpy_byte:
1:
	tst r2, r2	@ Check counter
	moveq pc, lr	@ Return if zero
	ldrb r3, [r1],#1	@ Read
	strb r3, [r0],#1	@ Write
	sub r2, #1
	b 1b

@ 
@ Pre-aligned memcpy (32-bit blocks)
@ 
.globl __memcpy_align4
__memcpy_align4:
	push {r4}
	mvn r3, #3	@ Mask for checking length
	
	@ 4 byte chunk copies
1:	tst r2, r3
	ldrne r4, [r1],#4
	strne r4, [r0],#4
	subne r2, #4
	bne 1b

	@ single byte copies to finish off
2:	tst r2, #3
	beq 3f
	ldrb r4, [r1],#1
	strb r4, [r0],#1
	sub r2, #1
	b 2b

3:	pop {r4}
	mov pc, lr

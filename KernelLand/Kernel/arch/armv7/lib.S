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

@
@ Division
@
.globl __divmod32_asm
__divmod32_asm:
	push {r4}
	mov r4, #0	@ Return value
	mov r3, #1	@ add value

	@ Scan up for first larger multiple of 2
1:	cmp r0, r1	@ N < D
	bmi 2f   	@ ^^
	lsl r1, r1, #1	@ D <<= 1
	lsls r3, r3, #1	@ add <<= 1
	beq .err	@ result is zero
	b 1b
	
	@ Go back down
2:	lsrs r3, r3, #1	@ add >>= 1
	beq 3f  	@ Done (value is zero)
	lsr r1, r1, #1	@ D >>= 1
	cmp r0, r1	@ N < D
	bmi 2b
	sub r0, r1	@ N -= D
	add r4, r3	@ ret += add
	b 2b
3:
	tst r2, r2	@ Remainder (if wanted)
	strne r0,[r2]
	mov r0, r4	@ Return value
	pop {r4}
	mov pc, lr
.err:
	mov r0, #0
	tst r2, r2
	strne r0, [r2]
	pop {r4}
	mov pc, lr

.globl __sync_fetch_and_and_4
__sync_fetch_and_and_4:
	mov r2, r0
	ldr r0, [r2]
	mov r3, r0
	and r3, r1
	str r3, [r2]
	mov pc, lr

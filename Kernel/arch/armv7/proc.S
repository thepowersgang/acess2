/*
 * Acess2 ARM
 * - By John Hodge (thePowersGang)
 *
 * arch/arm7/proc.S
 * - Process management assembly
 */

#include "include/assembly.h"

.globl KernelThreadHeader
@ SP+12: Argument 1
@ SP+8: Argument Count
@ SP+4: Function
@ SP+0: Thread Pointer
KernelThreadHeader:
	ldr r0, [sp],#4
	@ TODO: Do something with the thread pointer
	
	ldr r4, [sp],#4	@ Function
	ldr r5, [sp],#4
	@ Get arguments
	sub r5, #1
	ldrhs r0, [sp],#4
@	suble r5, #1
@	ldrhs r1, [sp],#4
@	suble r5, #1
@	ldrhs r2, [sp],#4
@	suble r5, #1
@	ldrhs r3, [sp],#4

	blx r4
	
	ldr r0, =0
	bl Threads_Exit
	b .

.globl SwitchTask
@ R0: New stack
@ R1: Pointer to where to save old stack
@ R2: New IP
@ R3: Pointer to save old IP
@ SP+0: New address space
SwitchTask:
	PUSH_GPRS

	@ Save IP	
	ldr r4, =.return
	str r4, [r3]
	@ Save SP
	str sp, [r1]

	@ Only update TTBR0 if the task has an explicit address space
	ldr r1, [sp,#0x40]
	tst r1, r1
	mcrne p15, 0, r1, c2, c0, 0	@ Set TTBR0 to r0

	@ Restore SP
	mov sp, r0

	bx r2

.return:
	POP_GPRS
	bx lr


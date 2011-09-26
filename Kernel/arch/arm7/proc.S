/*
 * Acess2 ARM
 * - By John Hodge (thePowersGang)
 *
 * arch/arm7/proc.S
 * - Process management assembly
 */

#define PUSH_GPRS \
	str r0, [sp,#-1*4];\
	str r1, [sp,#-2*4];\
	str r2, [sp,#-3*4];\
	str r3, [sp,#-4*4];\
	str r4, [sp,#-5*4];\
	str r5, [sp,#-6*4];\
	str r6, [sp,#-7*4];\
	str r7, [sp,#-8*4];\
	str r8, [sp,#-9*4];\
	str r9, [sp,#-10*4];\
	str r10, [sp,#-11*4];\
	str r11, [sp,#-12*4];\
	str r12, [sp,#-13*4];\
	str sp, [sp,#-14*4];\
	str lr, [sp,#-15*4];\
	sub sp, #16*4

#define POP_GPRS add sp, #16*4; \
	ldr r0, [sp,#-1*4]; \
	ldr r1, [sp,#-2*4]; \
	ldr r2, [sp,#-3*4]; \
	ldr r3, [sp,#-4*4]; \
	ldr r4, [sp,#-5*4]; \
	ldr r5, [sp,#-6*4]; \
	ldr r6, [sp,#-7*4]; \
	ldr r7, [sp,#-8*4]; \
	ldr r8, [sp,#-9*4]; \
	ldr r9, [sp,#-10*4]; \
	ldr r10, [sp,#-11*4]; \
	ldr r11, [sp,#-12*4]; \
	ldr r12, [sp,#-13*4]; \
	ldr lr, [sp,#-15*4];

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
	sub r5, #1
	ldrhs r1, [sp],#4
	sub r5, #1
	ldrhs r2, [sp],#4
	sub r5, #1
	ldrhs r3, [sp],#4

	mov lr, pc
	mov pc, r4
	
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
	
	ldr r4, =.return
	str r4, [r3]
	str sp, [r1]
	mov r0, sp
	
	@ Only update TTBR0 if the task has an explicit address space
	ldr r0, [sp,#0x40]
	tst r0, r0
	mcrne p15, 0, r0, c2, c0, 0	@ Set TTBR0 to r0

	mov pc, r2

.return:
	POP_GPRS
	bx lr


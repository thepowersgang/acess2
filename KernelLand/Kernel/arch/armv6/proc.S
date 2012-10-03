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
	@ Get argument
	ldr r0, [sp],#4

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
	push {r4-r12,lr}

	@ Save IP	
	ldr r4, =.return
	str r4, [r3]
	@ Save SP
	str sp, [r1]

	@ Only update TTBR0 if the task has an explicit address space
	ldr r1, [sp,#4*10]
	tst r1, r1
	mcrne p15, 0, r1, c2, c0, 0	@ Set TTBR0 to r0
#	mov r1, #1
	mcrne p15, 0, r1, c8, c7, 0	@ TLBIALL - Invalid user space

	@ Restore state
	mov sp, r0
	bx r2

.return:
	pop {r4-r12,pc}

.extern MM_Clone
.extern MM_DumpTables
.globl Proc_CloneInt
Proc_CloneInt:
	@ R0: SP Destination
	@ R1: Mem Destination
	push {r4-r12,lr}
	mov r4, r1	@ Save mem destination
	str sp, [r0]	@ Save SP to SP dest

	bl MM_Clone
	str r0, [r4]	@ Save clone return to Mem Dest

	ldr r0, =Proc_CloneInt_new
	pop {r4-r12,pc}
Proc_CloneInt_new:
	mov r0, #0
	pop {r4-r12,pc}

@ R0: New user SP
@ Return: Old user SP
.globl Proc_int_SwapUserSP
Proc_int_SwapUserSP:
	cps #31	@ Go to system mode
	mov r1, sp
	tst r0, r0	@ Only update if non-zero
	movne sp, r0
	mov r0, r1
	cps #19
	mov pc, lr

.section .usertext, "ax"
.globl Proc_int_DropToUser
@ R0: User IP
@ R1: User SP
Proc_int_DropToUser:
	cps #16
	mov sp, r1
	mov pc, r0

.section .rodata
csProc_CloneInt_NewTaskMessage:
	.asciz "New task PC=%p, R4=%p, sp=%p"
csProc_CloneInt_OldTaskMessage:
	.asciz "Parent task PC=%p, R4=%p, SP=%p"

@ vim: ft=armv7

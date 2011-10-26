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

.globl Proc_int_DropToUser
@ R0: User IP
@ R1: User SP
Proc_int_DropToUser:
	cps #16
	mov sp, r1
	mov pc, r0

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
	mov r1, #0
	mcrne p15, 0, r1, c8, c7, 0	@ Invalidate all (HACK! But it fixes things)

	@ Restore SP
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
@	cps #18
		
@	mov r0, #0
@	mvn r1, #0
@	bl MM_DumpTables

@	ldr r0, =csProc_CloneInt_NewTaskMessage
@	bl Log
	
@	cps #19
	mov r0, #0
	pop {r4-r12,pc}


.section .rodata
csProc_CloneInt_NewTaskMessage:
	.asciz "New task"

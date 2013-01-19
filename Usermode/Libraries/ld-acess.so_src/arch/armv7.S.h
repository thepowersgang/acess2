//
// Acess2 ARMv7 - System Calls
//

.globl _start
.extern SoMain
_start:
	pop {r0}
	ldm sp, {r1,r2,r3}
	bl SoMain
	
	mov r4, r0

	pop {r0,r1,r2}
	blx r4
	
	b _exit

@ Stupid GCC
.globl __ucmpdi2
__ucmpdi2:
	cmp r0, r2
	movmi r0, #0
	movmi pc, lr
	movhi r0, #2
	movhi pc, lr
	cmp r1, r2
	movmi r0, #0
	movmi pc, lr
	movhi r0, #2
	movhi pc, lr
	mov r0, #1
	mov pc, lr

@ Well, can't blame it
@ - Clear the instruction cache
.globl __clear_cache
__clear_cache:
	svc #0x1001
	mov pc, lr


@ >r0: PC
@ >r1: Pointer to item count
@ <r0: Address
@ STUBBED
__gnu_Unwind_Find_exidx:
	mov r0, #0
	str r0, [r1]
	mov pc, lr

.section .data
.globl _errno
_errno:	.long	0	@ Placed in .text, to allow use of relative addressing
.section .text

.macro syscall0 _name, _num	
.globl \_name
\_name:
	push {lr}
	svc #\_num
	@mrc p15, 0, r3, c13, c0, 2
	ldr r3, =_errno
	str r2, [r3]
	pop {pc}
.endm

.macro syscall5 _name, _num
.globl \_name
\_name:
	push {r4, lr}
	ldr r4, [sp,#8]
	svc #\_num
	ldr r3, =_errno
	str r2, [r3]
	pop {r4, pc}
.endm

.macro syscall6 _name, _num
.globl \_name
\_name:
	push {r4,r5,lr}
	ldr r4, [sp,#12]
	ldr r5, [sp,#16]
	svc #\_num
	ldr r3, =_errno
	str r2, [r3]
	pop {r4,r5,pc}
.endm

#define SYSCALL0(_name,_num)	syscall0 _name, _num
#define SYSCALL1(_name,_num)	SYSCALL0(_name, _num)
#define SYSCALL2(_name,_num)	SYSCALL0(_name, _num)
#define SYSCALL3(_name,_num)	SYSCALL0(_name, _num)
#define SYSCALL4(_name,_num)	SYSCALL0(_name, _num)
// TODO: 5/6 need special handling, because the args are on the stack
#define SYSCALL5(_name,_num)	syscall5 _name, _num
#define SYSCALL6(_name,_num)	syscall6 _name, _num

// Override the clone syscall
#define _exit	_exit_raw
#define _clone	_clone_raw
#define _SysSeek	_SysSeek_borken
#include "syscalls.s.h"
#undef _exit
#undef _clone
#undef _SysSeek 

// NOTE: _SysSeek needs special handling for alignment
.globl _SysSeek
_SysSeek:
	push {lr}
	mov r1,r2
	mov r2,r3
	ldr r3, [sp,#4]
	svc #SYS_SEEK
	ldr r3, =_errno
	str r2, [r3]
	pop {pc}

.globl _clone
_clone:
	push {r4}
	mov r4, r1
	svc #SYS_CLONE
	ldr r3, =_errno
	str r2, [r3]
	tst r4, r4
	beq _clone_ret
	@ If in child, set SP
	tst r0,r0
	movne sp, r4
_clone_ret:
	pop {r4}
	mov pc, lr


.globl _exit
_exit:
	svc #0
	b .


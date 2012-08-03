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

@ DEST
@ SRC
@_memcpy:
@	push rbp
@	mov rbp, rsp
@	
@	; RDI - First Param
@	; RSI - Second Param
@	mov rcx, rdx	; RDX - Third
@	rep movsb
@	
@	pop rbp
@	ret
@
.globl _errno
_errno:	.long	0	@ Placed in .text, to allow use of relative addressing

.macro syscall0 _name, _num	
.globl \_name
\_name:
	push {lr}
	svc #\_num
	str r2, _errno
	pop {pc}
.endm

.macro syscall5 _name, _num
.globl \_name
\_name:
	push {r4, lr}
	ldr r4, [sp,#8]
	svc #\_num
	str r2, _errno
	pop {r4, pc}
.endm

.macro syscall6 _name, _num
.globl \_name
\_name:
	push {r4,r5,lr}
	ldr r4, [sp,#12]
	ldr r5, [sp,#16]
	svc #\_num
	str r2, _errno
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
#include "syscalls.s.h"
#undef _exit
#undef _clone

.globl _clone
_clone:
	push {r4}
	mov r4, r1
	svc #SYS_CLONE
	str r2, _errno
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

.globl abort
abort:
	mov r0, #0
	svc #0
	b .

.globl __exidx_start
__exidx_start:
	b .
.globl __exidx_end
__exidx_end:
	b .


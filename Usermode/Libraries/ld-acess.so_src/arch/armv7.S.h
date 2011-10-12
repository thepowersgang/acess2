//
// Acess2 ARMv7 - System Calls
//

#include "../../../../Kernel/include/syscalls.h"

.globl _start
.extern SoMain
_start:
	push {r1,r2,r3}
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
	svc #\_num
	str r2, _errno
	mov pc, lr
.endm

#define SYSCALL0(_name,_num)	syscall0 _name, _num
#define SYSCALL1(_name,_num)	SYSCALL0(_name, _num)
#define SYSCALL2(_name,_num)	SYSCALL0(_name, _num)
#define SYSCALL3(_name,_num)	SYSCALL0(_name, _num)
#define SYSCALL4(_name,_num)	SYSCALL0(_name, _num)
// TODO: 5/6 need special handling, because the args are on the stack
#define SYSCALL5(_name,_num)	SYSCALL0(_name, _num)
#define SYSCALL6(_name,_num)	SYSCALL0(_name, _num)

// Override the clone syscall
#define _exit	_exit_raw
#include "syscalls.s.h"
#undef _exit

.globl _exit
_exit:
	svc #0
	b .


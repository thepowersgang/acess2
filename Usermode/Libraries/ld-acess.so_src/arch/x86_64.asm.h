; ========================
; AcssMicro - System Calls
; ========================

%include "../../../Kernel/include/syscalls.inc.asm"

[bits 64]

[section .text]
; DEST
; SRC
_memcpy:
	push rbp
	mov rbp, rsp
	push rdi
	push rsi	; // DI and SI must be maintained, CX doesn't
	
	mov rcx, [rbp+4*8]
	mov rsi, [rbp+3*8]
	mov rdi, [rbp+2*8]
	rep movsb
	
	pop rsi
	pop rdi
	pop rbp
	ret

[global _errno:data 4]
_errno:	dw	0	; Placed in .text, to allow use of relative addressing

#define SYSCALL0(_name,_num)	SYSCALL0 _name, _num
#define SYSCALL1(_name,_num)	SYSCALL1 _name, _num
#define SYSCALL2(_name,_num)	SYSCALL2 _name, _num
#define SYSCALL3(_name,_num)	SYSCALL3 _name, _num
#define SYSCALL4(_name,_num)	SYSCALL4 _name, _num
#define SYSCALL5(_name,_num)	SYSCALL5 _name, _num
#define SYSCALL6(_name,_num)	SYSCALL6 _name, _num

;%define SYSCALL_OP	jmp 0xCFFF0000
%define SYSCALL_OP	int 0xAC

; System Call - No Arguments
%macro SYSCALL0	2
[global %1:func]
%1:
	push rbx
	mov eax, %2
	SYSCALL_OP
	mov [DWORD rel _errno], ebx
	pop rbx
	ret
%endmacro

%macro _SYSCALL_HEAD 2
[global %1:func]
%1:
	push rbp
	mov rbp, rsp
	push rbx
	mov eax, %2
%endmacro
%macro _SYSCALL_TAIL 0
	mov [DWORD rel _errno], ebx
	pop rbx
	pop rbp
	ret
%endmacro

; System Call - 1 Argument
%macro SYSCALL1	2
_SYSCALL_HEAD %1, %2
	mov rbx, [rbp+2*8]
	SYSCALL_OP
_SYSCALL_TAIL
%endmacro

; System Call - 2 Arguments
%macro SYSCALL2	2
_SYSCALL_HEAD %1, %2
	mov rbx, [rbp+2*8]
	mov rcx, [rbp+3*8]
	SYSCALL_OP
_SYSCALL_TAIL
%endmacro

; System Call - 3 Arguments
%macro SYSCALL3	2
_SYSCALL_HEAD %1, %2
	mov rbx, [rbp+2*8]
	mov rcx, [rbp+3*8]
	mov rdx, [rbp+4*8]
	SYSCALL_OP
_SYSCALL_TAIL
%endmacro

; System Call - 4 Arguments
%macro SYSCALL4	2
_SYSCALL_HEAD %1, %2
	push rdi
	mov rbx, [rbp+2*8]
	mov rcx, [rbp+3*8]
	mov rdx, [rbp+4*8]
	mov rdi, [rbp+5*8]
	SYSCALL_OP
	pop rdi
_SYSCALL_TAIL
%endmacro

; System Call - 5 Arguments
%macro SYSCALL5	2
_SYSCALL_HEAD %1, %2
	push rdi
	push rsi
	mov rbx, [rbp+2*8]
	mov rcx, [rbp+3*8]
	mov rdx, [rbp+4*8]
	mov rdi, [rbp+5*8]
	mov rsi, [rbp+6*8]
	SYSCALL_OP
	pop rsi
	pop rdi
_SYSCALL_TAIL
%endmacro

; System Call - 6 Arguments
%macro SYSCALL6	2
_SYSCALL_HEAD %1, %2
	push rdi
	push rsi
	mov rbx, [rbp+2*8]
	mov rcx, [rbp+3*8]
	mov rdx, [rbp+4*8]
	mov rdi, [rbp+5*8]
	mov rsi, [rbp+6*8]
	mov rbp, [rbp+7*8]
	SYSCALL_OP
	pop rsi
	pop rdi
_SYSCALL_TAIL
%endmacro

; // Override the clone syscall
#define clone	_clone_raw
#include "syscalls.s.h"
#undef clone

[global clone:func]
clone:
	push rbp
	mov rbp, rsp
	push rbx
	
	mov rbx, [rbp+3*8]	; Get new stack pointer
	
	; Check if the new stack is being used
	test rbx, rbx
	jz .doCall
	; Quick hack, just this stack frame
	mov rax, [rbp+1*8]
	mov [rbx-1*8], rax	; Return
	mov [rbx-2*8], rbx	; EBP
	and QWORD [rbx-3*8], BYTE 0	; EBX
	sub rbx, 3*8
.doCall:
	mov eax, SYS_CLONE
	mov rcx, rbx	; Stack
	mov rbx, [rbp+2*8]	; Flags
	SYSCALL_OP
	mov [rel _errno], ebx
	pop rbx
	pop rbp
	ret


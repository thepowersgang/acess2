; ========================
; AcssMicro - System Calls
; ========================

%include "../../../Kernel/include/syscalls.inc.asm"

[bits 64]

[section .text]
[global _start]
[extern SoMain]
_start:
	pop rdi
	call SoMain
	
	mov rdi, [rsp]
	mov rsi, [rsp+8]
	mov rdx, [rsp+16]
	call rax
	
	mov rdi, rax
	call _exit

; DEST
; SRC
_memcpy:
	push rbp
	mov rbp, rsp
	
	; RDI - First Param
	; RSI - Second Param
	mov rcx, rdx	; RDX - Third
	rep movsb
	
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
;%define SYSCALL_OP	int 0xAC
%define SYSCALL_OP	syscall

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
;	mov rdi, [rbp+3*8]
	SYSCALL_OP
_SYSCALL_TAIL
%endmacro

; System Call - 2 Arguments
%macro SYSCALL2	2
_SYSCALL_HEAD %1, %2
;	mov rdi, [rbp+3*8]
;	mov rsi, [rbp+4*8]
	SYSCALL_OP
_SYSCALL_TAIL
%endmacro

; System Call - 3 Arguments
%macro SYSCALL3	2
_SYSCALL_HEAD %1, %2
;	mov rdi, [rbp+3*8]
;	mov rsi, [rbp+4*8]
;	mov rdx, [rbp+5*8]
	SYSCALL_OP
_SYSCALL_TAIL
%endmacro

; System Call - 4 Arguments
%macro SYSCALL4	2
_SYSCALL_HEAD %1, %2
;	mov rdi, [rbp+2*8]
;	mov rsi, [rbp+3*8]
;	mov rdx, [rbp+4*8]
	mov r10, rcx	; r10 is used in place of RCX
	SYSCALL_OP
_SYSCALL_TAIL
%endmacro

; System Call - 5 Arguments
%macro SYSCALL5	2
_SYSCALL_HEAD %1, %2
;	mov rdi, [rbp+2*8]
;	mov rsi, [rbp+3*8]
;	mov rdx, [rbp+4*8]
	mov r10, rcx
;	mov r8, [rbp+6*8]
	SYSCALL_OP
_SYSCALL_TAIL
%endmacro

; System Call - 6 Arguments
%macro SYSCALL6	2
_SYSCALL_HEAD %1, %2
;	mov rdi, [rbp+2*8]
;	mov rsi, [rbp+3*8]
;	mov rdx, [rbp+4*8]
	mov r10, rcx
;	mov r8, [rbp+6*8]
;	mov r9, [rbp+7*8]
	SYSCALL_OP
_SYSCALL_TAIL
%endmacro

; // Override the clone syscall
#define clone	_clone_raw
#define _exit	_exit_raw
#include "syscalls.s.h"
#undef clone
#undef _exit

[global clone:func]
clone:
	push rbp
	mov rbp, rsp
	push rbx
	push r11
	
	mov r12, rsi	; Save in a reg for after the clone
	
	; Check if the new stack is being used
	test rsi, rsi
	jz .doCall
	; Quick hack, just this stack frame
	mov rax, [rbp+1*8]
	mov [rsi-1*8], rax	; Return
	mov [rsi-2*8], rsi	; EBP
	and QWORD [rsi-3*8], BYTE 0	; EBX
	sub rsi, 3*8
.doCall:
	mov eax, SYS_CLONE
	SYSCALL_OP
	mov [rel _errno], ebx

	; Change stack pointer
	test eax, eax
	jnz .ret
	test r12, r12
	jz .ret
	mov rsp, rsi
.ret:
	pop rbx
	pop rbp
	ret


[global _exit:func]
_exit:
	xor eax, eax
	SYSCALL_OP
	jmp $

; vim: ft=nasm

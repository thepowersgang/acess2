; ========================
; AcssMicro - System Calls
; ========================

%include "../../../Kernel/include/syscalls.inc.asm"

[bits 32]
[section .data]
[global _errno:data 4]
_errno:	dw	0

[section .text]
[global _start]
[extern SoMain]
_start:
	call SoMain

	add esp, 4
	call eax

	push eax
	call _exit

; DEST
; SRC
_memcpy:
	push ebp
	mov ebp, esp
	push edi
	push esi	; // DI and SI must be maintained, CX doesn't
	
	mov ecx, [ebp+16]
	mov esi, [ebp+12]
	mov edi, [ebp+8]
	rep movsb
	
	pop esi
	pop edi
	pop ebp
	ret

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
	push ebx
	mov eax, %2
	SYSCALL_OP
	mov [_errno], ebx
	pop ebx
	ret
%endmacro

%macro _SYSCALL_HEAD 2
[global %1:func]
%1:
	push ebp
	mov ebp, esp
	push ebx
	mov eax, %2
%endmacro
%macro _SYSCALL_TAIL 0
	mov [_errno], ebx
	pop ebx
	pop ebp
	ret
%endmacro

; System Call - 1 Argument
%macro SYSCALL1	2
_SYSCALL_HEAD %1, %2
	mov ebx, [ebp+8]
	SYSCALL_OP
_SYSCALL_TAIL
%endmacro

; System Call - 2 Arguments
%macro SYSCALL2	2
_SYSCALL_HEAD %1, %2
	mov eax, %2
	mov ebx, [ebp+8]
	mov ecx, [ebp+12]
	SYSCALL_OP
_SYSCALL_TAIL
%endmacro

; System Call - 3 Arguments
%macro SYSCALL3	2
_SYSCALL_HEAD %1, %2
	mov ebx, [ebp+8]
	mov ecx, [ebp+12]
	mov edx, [ebp+16]
	SYSCALL_OP
_SYSCALL_TAIL
%endmacro

; System Call - 4 Arguments
%macro SYSCALL4	2
_SYSCALL_HEAD %1, %2
	push edi
	mov ebx, [ebp+8]
	mov ecx, [ebp+12]
	mov edx, [ebp+16]
	mov edi, [ebp+20]
	SYSCALL_OP
	pop edi
_SYSCALL_TAIL
%endmacro

; System Call - 5 Arguments
%macro SYSCALL5	2
_SYSCALL_HEAD %1, %2
	push edi
	push esi
	mov ebx, [ebp+8]
	mov ecx, [ebp+12]
	mov edx, [ebp+16]
	mov edi, [ebp+20]
	mov esi, [ebp+24]
	SYSCALL_OP
	pop esi
	pop edi
_SYSCALL_TAIL
%endmacro

; System Call - 6 Arguments
%macro SYSCALL6	2
_SYSCALL_HEAD %1, %2
	push edi
	push esi
	mov ebx, [ebp+8]
	mov ecx, [ebp+12]
	mov edx, [ebp+16]
	mov edi, [ebp+20]
	mov esi, [ebp+24]
	mov ebp, [ebp+28]
	SYSCALL_OP
	pop esi
	pop edi
_SYSCALL_TAIL
%endmacro

; // Override the clone syscall
#define clone	_clone_raw
#include "syscalls.s.h"
#undef clone

[global clone:func]
clone:
	push ebp
	mov ebp, esp
	push ebx
	
	mov ebx, [ebp+12]	; Get new stack pointer
	
	; Check if the new stack is being used
	test ebx, ebx
	jz .doCall
	; Modify it to include the calling function (and this)
	%if 0
	mov eax, [ebp]	; Get old stack frame
	sub eax, ebp	; Get size
	sub ebx, eax	; Alter new stack pointer
	push eax	; < Size
	push DWORD [ebp]	; < Source
	push ebx	; < Dest
	call _memcpy
	add esp, 4*3	; Restore stack
	; EBX should still be the new stack pointer
	mov eax, [ebp]	; Save old stack frame pointer in new stack
	mov [ebx-4], eax
	mov eax, [ebp-4]	; Save EBX there too
	mov [ebx-8], eax
	sub ebx, 8	; Update stack pointer for system
	%else
	; Quick hack, just this stack frame
	mov eax, [ebp+4]
	mov [ebx-4], eax	; Return
	mov [ebx-8], ebx	; EBP
	mov DWORD [ebx-12], 0	; EBX
	sub ebx, 12
	%endif
.doCall:
	mov eax, SYS_CLONE
	mov ecx, ebx	; Stack
	mov ebx, [ebp+8]	; Flags
	SYSCALL_OP
	mov [_errno], ebx
	pop ebx
	pop ebp
	ret


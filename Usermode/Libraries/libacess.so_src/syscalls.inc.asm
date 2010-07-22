; ========================
; AcssMicro - System Calls
; ========================

%include "../../../Kernel/include/syscalls.inc.asm"

SYSCALL_JUMP equ	0xCFFF0000

; System Call - No Arguments
%macro SYSCALL0	2
[global %1:func]
%1:
	push ebx
	mov eax, %2
	jmp SYSCALL_JUMP
	mov [_errno], ebx
	pop ebx
	ret
%endmacro

; System Call - 1 Argument
%macro SYSCALL1	2
[global %1:func]
%1:
	push ebp
	mov ebp, esp
	push ebx
	mov eax, %2
	mov ebx, [ebp+8]
	jmp SYSCALL_JUMP
	mov [_errno], ebx
	pop ebx
	pop ebp
	ret
%endmacro

; System Call - 2 Arguments
%macro SYSCALL2	2
[global %1:func]
%1:
	push ebp
	mov ebp, esp
	push ebx
	mov eax, %2
	mov ebx, [ebp+8]
	mov ecx, [ebp+12]
	jmp SYSCALL_JUMP
	mov [_errno], ebx
	pop ebx
	pop ebp
	ret
%endmacro

; System Call - 3 Arguments
%macro SYSCALL3	2
[global %1:func]
%1:
	push ebp
	mov ebp, esp
	push ebx
	mov eax, %2
	mov ebx, [ebp+8]
	mov ecx, [ebp+12]
	mov edx, [ebp+16]
	jmp SYSCALL_JUMP
	mov [_errno], ebx
	pop ebx
	pop ebp
	ret
%endmacro

; System Call - 4 Arguments
%macro SYSCALL4	2
[global %1:func]
%1:
	push ebp
	mov ebp, esp
	push ebx
	push edi
	mov eax, %2
	mov ebx, [ebp+8]
	mov ecx, [ebp+12]
	mov edx, [ebp+16]
	mov edi, [ebp+20]
	jmp SYSCALL_JUMP
	mov [_errno], ebx
	pop edi
	pop ebx
	pop ebp
	ret
%endmacro

; System Call - 5 Arguments
%macro SYSCALL5	2
[global %1:func]
%1:
	push ebp
	mov ebp, esp
	push ebx
	push edi
	push esi
	mov eax, %2
	mov ebx, [ebp+8]
	mov ecx, [ebp+12]
	mov edx, [ebp+16]
	mov edi, [ebp+20]
	mov esi, [ebp+24]
	jmp SYSCALL_JUMP
	mov [_errno], ebx
	pop esi
	pop edi
	pop ebx
	pop ebp
	ret
%endmacro

; System Call - 6 Arguments
%macro SYSCALL6	2
[global %1:func]
%1:
	push ebp
	mov ebp, esp
	push ebx
	push edi
	push esi
	mov eax, %2
	mov ebx, [ebp+8]
	mov ecx, [ebp+12]
	mov edx, [ebp+16]
	mov edi, [ebp+20]
	mov esi, [ebp+24]
	mov ebp, [ebp+28]
	jmp SYSCALL_JUMP
	mov [_errno], ebx
	pop esi
	pop edi
	pop ebx
	pop ebp
	ret
%endmacro

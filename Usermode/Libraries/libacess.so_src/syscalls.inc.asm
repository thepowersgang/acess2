; ========================
; AcssMicro - System Calls
; ========================

%include "../../../Kernel/include/syscalls.inc.asm"

; System Call - No Arguments
%macro SYSCALL0	2
[global %1:func]
%1:
	push ebx
	mov eax, %2
	int 0xAC
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
	int 0xAC
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
	int 0xAC
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
	int 0xAC
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
	int 0xAC
	mov [_errno], ebx
	pop edi
	pop ebx
	pop ebp
	ret
%endmacro

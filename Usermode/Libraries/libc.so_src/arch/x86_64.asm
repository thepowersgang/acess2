; 
; Acess2 C Library
; - By John Hodge (thePowersGang)
; 
; arch/x86_64.asm
; - x86_64 specific code
[bits 64]
[section .text]

[global setjmp]
setjmp:
	;mov [rdi+0x00], rax
	mov [rdi+0x08], rcx
	mov [rdi+0x10], rdx
	mov [rdi+0x18], rbx
	mov [rdi+0x20], rsi
	mov [rdi+0x28], rdi
	mov [rdi+0x30], rsp
	mov [rdi+0x38], rbp
	mov [rdi+0x40], r8
	mov [rdi+0x48], r9
	mov [rdi+0x50], r10
	mov [rdi+0x58], r11
	mov [rdi+0x60], r12
	mov [rdi+0x68], r13
	mov [rdi+0x70], r14
	mov [rdi+0x78], r15
	
	xor eax, eax
	ret
setjmp.restore:
	ret

[global longjmp]
longjmp:
	mov rax, rsi
	
	;mov rax, [rdi+0x00]
	mov rcx, [rdi+0x08]
	mov rdx, [rdi+0x10]
	mov rbx, [rdi+0x18]
	mov rsi, [rdi+0x20]
	mov rdi, [rdi+0x28]
	mov rsp, [rdi+0x30]
	mov rbp, [rdi+0x38]
	mov r8,  [rdi+0x40]
	mov r9,  [rdi+0x48]
	mov r10, [rdi+0x50]
	mov r11, [rdi+0x58]
	mov r12, [rdi+0x60]
	mov r13, [rdi+0x68]
	mov r14, [rdi+0x70]
	mov r15, [rdi+0x78]
	
	test eax, eax
	jnz .ret
	inc eax

	; Return to where setjmp was called
.ret:
	ret


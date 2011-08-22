;
; Acess2
; C Runtime 0
; - crt0.asm

[BITS 64]
[section .text]


[global _start]
[global start]
[extern main]
[extern _exit]
_start:
start:
	call main
	push rax

	mov rax, [_crt0_exit_handler]
	test rax, rax
	jz .exit
	call rax

.exit:
	call _exit
	jmp $	; This should never be reached

[section .bss]
[global _crt0_exit_handler]
_crt0_exit_handler:
	resq	1

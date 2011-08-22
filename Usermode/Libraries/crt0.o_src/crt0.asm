;
; Acess2
; C Runtime 0
; - crt0.asm

[BITS 32]
[section .text]


[global _start]
[global start]
[extern main]
[extern _exit]
_start:
start:
	call main
	push eax

	mov eax, [_crt0_exit_handler]
	test eax, eax
	jz .exit
	call eax
	
.exit:
	call _exit
	jmp $	; This should never be reached
[section .bss]
[global _crt0_exit_handler]
_crt0_exit_handler:
	resd	1

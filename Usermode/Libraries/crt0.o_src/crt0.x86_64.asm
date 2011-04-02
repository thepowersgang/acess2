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
	call _exit
	jmp $	; This should never be reached

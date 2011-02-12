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
	call _exit
	jmp $	; This should never be reached

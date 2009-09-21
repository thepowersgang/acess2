;
; Acess2
; C Runtime 0
; - crt0.asm

[BITS 32]
[section .text]


[global _start]
[global start]
[extern main]
_start:
start:
	call main
	mov eax, ebx	; Set Argument 1 to Return Value
	xor eax, eax	; Set EAX to SYS_EXIT (0)
	int	0xAC
	jmp $	; This should never be reached

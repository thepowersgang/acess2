;
; Acess2 x86_64 Port
;
[bits 64]

[section .text]
[global start64]
start64:
	; Set kernel stack
	; Call main
	jmp $

[global GetRIP]
GetRIP:
	mov rax, [rsp]
	ret

[section .bss]
[global gInitialKernelStack]
	resd	1024*1	; 1 Page
gInitialKernelStack:

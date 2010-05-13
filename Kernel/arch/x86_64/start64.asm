;
; Acess2 x86_64 Port
;
[bits 64]

[global start64]
start64:
	; Set kernel stack
	; Call main
	jmp $

[global GetRIP]
GetRIP:
	mov rax, [rsp]
	ret

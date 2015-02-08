%macro PUSH_CC	0
	; Don't bother being too picky, just take the time
	pusha
%endmacro
%macro PUSH_SEG	0
	push ds
	push es
	push fs
	push gs
	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
%endmacro
%macro POP_CC	0
	popa
%endmacro
%macro POP_SEG	0
	pop gs
	pop fs
	pop es
	pop ds
%endmacro

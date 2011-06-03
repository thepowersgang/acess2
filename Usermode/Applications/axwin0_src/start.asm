; AcessBasic Test Command Prompt
; Assembler Stub

[BITS 32]

extern _main
extern codeLength
extern loadTo
extern entrypoint
extern magic

global start
global	_gAxeHdr

FLAGS equ 0
MAXMEM	equ	0x400000	; 4Mb

;Header
db	'A'
db	'X'
db	'E'
db	0
;Size
_gAxeHdr:
dd codeLength	;Code Size
dd loadTo		;Load Address
dd entrypoint	;Entrypoint
dd MAXMEM	;Maximum Used Memory
dd FLAGS	;Flags
dd magic+FLAGS+MAXMEM

;Code
start:
	push eax
	call _main
	
	ret
	ret
	
;String Compare
_strcmp:
	push ebp
	mov ebp, esp
	push ebx
	push ecx
	
	mov ebx, [ebp+8]
	mov ecx, [ebp+12]
.cmp:
	mov al, BYTE [ecx]
	cmp BYTE [ebx], al
	jnz	.out
	cmp BYTE [ecx],0
	jnz	.out
	inc ebx
	inc edx
	jmp	.cmp
.out:
	mov eax, DWORD 0
	mov al, BYTE [ebx]
	sub al, BYTE [ecx]
	;Cleanup
	pop	ecx
	pop ebx
	pop ebp
	ret
	
;String Copy
_strcpy:
	push ebp
	mov ebp, esp
	push ebx
	push ecx
	push edx
	mov ebx, [ebp+8]	;Src
	mov ecx, [ebp+12]	;Dest
.cmp:
	cmp BYTE [ebx], 0
	jnz .out
	mov dl, BYTE [ebx]
	mov BYTE [ecx], dl
	inc ebx
	inc ecx
	jmp .cmp
.out:	;Cleanup
	pop edx
	pop ecx
	pop ebx
	pop ebp
	ret

	
[section .bss]
[global _startHeap]
[global _endHeap]
_startHeap:
resb 0x4000	;Heap Space
_endHeap:

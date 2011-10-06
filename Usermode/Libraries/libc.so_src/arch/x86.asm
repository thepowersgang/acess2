; 
; Acess2 C Library
; - By John Hodge (thePowersGang)
; 
; arch/x86.asm
; - x86 specific code
[bits 32]
[section .text]

[global setjmp]
setjmp:
	mov eax, [esp+4]	; Get base of buffer
	
	mov [eax+0x00], eax
	mov [eax+0x04], ecx
	mov [eax+0x08], edx
	mov [eax+0x0C], ebx
	mov [eax+0x10], esi
	mov [eax+0x14], edi
	mov [eax+0x18], esp
	mov [eax+0x1C], ebp
	
	xor eax, eax
	ret
setjmp.restore:
	ret

[global longjmp]
longjmp:
	mov ebp, [esp+4]	; jmp_buf
	mov eax, [esp+8]	; value
	
	;mov eax, [ebp+0x00]
	mov ecx, [ebp+0x04]
	mov edx, [ebp+0x08]
	mov ebx, [ebp+0x0C]
	mov esi, [ebp+0x10]
	mov edi, [ebp+0x14]
	mov esp, [ebp+0x18]
	mov ebp, [ebp+0x1C]
	
	test eax, eax
	jnz .ret
	inc eax

	; Return to where setjmp was called
.ret:
	ret


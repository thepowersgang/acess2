
global _k_fopen
global _k_fclose
global _k_ftell
global _k_fseek
global _k_fread
global _k_fwrite
global _k_fstat
;global _k_printf
global _k_puts
global _k_opendir
global _k_readdir
global _k_exec
global _k_ioctl

%macro	START_FRAME 0
	push ebp
	mov ebp, esp
	push ebx
	push ecx
	push edx
%endmacro
%macro	END_FRAME 0
	pop edx
	pop ecx
	pop ebx
	pop ebp
%endmacro

_k_fopen:
	START_FRAME
	mov eax, 2	;SYS_OPEN
	mov ebx, DWORD [ebp+8]
	mov ecx, DWORD [ebp+12]
	mov edx, 0
	int 0xAC
	END_FRAME
	ret
	;jmp	0x0010001c
_k_fclose:
	START_FRAME
	mov eax, 3	;SYS_CLOSE
	mov ebx, DWORD [ebp+8]
	int	0xAC
	END_FRAME	;Restore Stack Frame
	ret
	;jmp	0x00100022
_k_ftell:
	jmp	0x0010002e
_k_fseek:
	jmp	0x001000a9
_k_fread:
	START_FRAME
	mov eax, 4	;SYS_READ
	mov ebx, DWORD [ebp+8]
	mov ecx, DWORD [ebp+12]
	mov edx, DWORD [ebp+16]
	int 0xAC
	END_FRAME
	ret
_k_fwrite:
	START_FRAME
	mov eax, 5	;SYS_WRITE
	mov ebx, DWORD [ebp+8]
	mov ecx, DWORD [ebp+12]
	mov edx, DWORD [ebp+16]
	int 0xAC
	END_FRAME
	ret
_k_fstat:
	START_FRAME
	mov eax, 10	;SYS_FSTAT
	mov ebx, DWORD [ebp+8]
	mov ecx, DWORD [ebp+12]
	int 0xAC
	END_FRAME
	ret
;_k_printf:
;		jmp	0x1000a4
_k_puts:
		jmp	0x105418
_k_opendir:
	START_FRAME
	mov eax, 2	;SYS_OPEN
	mov ebx, DWORD [ebp+8]
	mov ecx, 0
	mov edx, 0
	int 0xAC
	END_FRAME
	ret
	;jmp	0x00100050
_k_readdir:
	START_FRAME
	mov eax, 11	;SYS_READDIR
	mov ebx, DWORD [ebp+8]
	mov ecx, DWORD [ebp+12]
	int 0xAC
	END_FRAME
	ret

_k_exec:
	START_FRAME
	mov eax, 14	;SYS_EXEC
	mov ebx, DWORD [ebp+8]
	int 0xAC
	END_FRAME
	ret

_k_ioctl:
	START_FRAME
	mov eax, 12	;SYS_IOCTL
	mov ebx, DWORD [ebp+8]
	mov ecx, DWORD [ebp+12]
	mov edx, DWORD [ebp+16]
	int 0xAC
	END_FRAME
	ret

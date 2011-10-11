;
; Acess2 x86_64 Port
;
%include "arch/x86_64/include/common.inc.asm"
[bits 64]
;KERNEL_BASE	equ	0xFFFF800000000000
KERNEL_BASE	equ	0xFFFFFFFF80000000

[extern kmain]

[extern gMultibootPtr]
[extern gMultibootMagic]

[section .text]
[global start64]
start64:
	; Load Registers
	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	
	; Go to high memory
	mov rax, start64.himem
	jmp rax
.himem:
	
	xor rax, rax
	mov dr0, rax	; Set CPU0
	
	; Clear the screen
	mov rax, 0x1F201F201F201F20	; Set the screen to White on blue, space (4 characters)
	mov edi, 0xB8000
	mov ecx, 80*25*2/8
	rep stosq
	
	; Set kernel stack
	mov rsp, 0xFFFFA00000000000 + INITIAL_KSTACK_SIZE*0x1000
	
	; Call main
	mov edi, [gMultibootMagic - KERNEL_BASE]
	mov esi, [gMultibootPtr - KERNEL_BASE]
	call kmain
	
	cli
.hlt:
	hlt
	jmp .hlt

[global GetCPUNum]
GetCPUNum:
	xor rax, rax
	str ax
	sub ax, 0x38	; TSS Base
	shr ax, 4	; One 16-byte TSS per CPU
	ret

KSTACK_USERSTATE_SIZE	equ	(5+2+16+2)*8	; IRET, ErrorNum, ErrorCode, GPRs, FS&GS
[global Proc_ReturnToUser]
Proc_ReturnToUser:
	; RDI - Handler
	; RSI - Kernel Stack
	; RDX - Signal num
	
	;
	; NOTE: This can cause corruption if the signal happens while the user
	;       has called a kernel operation.
	; Good thing this can only be called on a user fault.
	;

	xchg bx, bx	
	; Get and alter User SP
	mov rcx, [rsi-0x20]	; Get user SP
	xor eax, eax
	mov [rcx-16], rax
	sub rcx, 16
	
	; Drop down to user mode
	cli
	mov rsp, rcx	; Set SP
	mov rcx, rdi	; SYSRET IP
	
	mov rdi, rdx	; Argument for handler
	mov r11, 0x202	; RFlags
	db 0x48
	sysret

; int CallWithArgArray(void *Ptr, int NArgs, Uint *Args)
; Call a function passing the array as arguments
[global CallWithArgArray]
CallWithArgArray:
	push rbp
	mov rbp, rsp
	push r10
	push r11
	
	mov [rbp+2*8], rdi	; Save Ptr to stack
	
	mov r11, rsi	; NArgs
	mov r10, rdx	; Args

	; Arg 1: RDI
	mov rdi, [r10]
	add r10, 8
	dec r11
	jz .call
	; Arg 2: RSI
	mov rsi, [r10]
	add r10, 8
	dec r11
	jz .call
	; Arg 3: RDX
	mov rdx, [r10]
	add r10, 8
	dec r11
	jz .call
	; Arg 4: RCX
	mov rcx, [r10]
	add r10, 8
	dec r11
	jz .call
	; Arg 5: R8
	mov r8, [r10]
	add r10, 8
	dec r11
	jz .call
	; Arg 6: R9
	mov r9, [r10]
	add r10, 8
	dec r11
	jz .call
	; No support for more

.call:
	mov rax, [rbp+2*8]	; Ptr
	call rax
	
	pop r11
	pop r10
	
	lea rsp, [rbp]
	pop rbp
	ret

; vim: ft=nasm

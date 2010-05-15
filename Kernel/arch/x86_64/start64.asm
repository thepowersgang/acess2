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

[global GetCPUNum]
GetCPUNum:
	str ax
	mov gs, ax
	xor rax, rax
	mov al, [gs:104]	; End of TSS
	ret

KSTACK_USERSTATE_SIZE	equ	(16+1+5)*8	; GPRegs, CPU, IRET
[global Proc_ReturnToUser]
[extern Proc_GetCurThread]
Proc_ReturnToUser:
	; RBP is the handler to use
	
	call Proc_GetCurThread
	
	; EAX is the current thread
	mov rbx, rax
	mov rax, [rbx+40]	; Get Kernel Stack
	sub rax, KSTACK_USERSTATE_SIZE
	
	;
	; NOTE: This can cause corruption if the signal happens while the user
	;       has called a kernel operation.
	; Good thing this can only be called on a user fault.
	;
	
	; Get and alter User SP
	mov rcx, [rax+KSTACK_USERSTATE_SIZE-3*8]
	mov rdx, [rbx+60]	; Get Signal Number
	mov [rcx-8], rdx
	mov rax, User_Syscall_RetAndExit
	mov [rcx-16], rax
	sub rcx, 16
	
	; Restore Segment Registers
	mov ax, 0x23
	mov ds, ax
	mov es, ax
	
	push 0x23	; SS
	push rcx	; RSP
	push 0x202	; RFLAGS (IF and Rsvd)
	push 0x1B	; CS
	push rbp	; RIP
	
	iret

[section .usertext]
User_Syscall_RetAndExit:
	mov rdi, rax
	jmp User_Syscall_Exit
User_Syscall_Exit:
	xor rax, rax
	; RDI: Return Value
	int 0xAC

[section .bss]
[global gInitialKernelStack]
	resd	1024*1	; 1 Page
gInitialKernelStack:


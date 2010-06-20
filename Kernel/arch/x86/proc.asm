; AcessOS Microkernel Version
; Start.asm

[bits 32]

KERNEL_BASE	equ 0xC0000000

KSTACK_USERSTATE_SIZE	equ	(4+8+1+5)*4	; SRegs, GPRegs, CPU, IRET

[section .text]
; --------------
; Task Scheduler
; --------------
[extern Proc_Scheduler]
[global SchedulerBase]
SchedulerBase:
	pusha
	push ds
	push es
	push fs
	push gs
	
	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	
	mov eax, [esp+12*4]	; CPU Number
	push eax	; Pus as argument
	
	call Proc_Scheduler
	
	add esp, 4	; Remove Argument
	
	pop gs
	pop fs
	pop es
	pop ds

	mov dx, 0x20
	mov al, 0x20
	out dx, al		; ACK IRQ
	popa
	add esp, 4	; CPU ID
	; No Error code / int num
	iret

[extern Proc_Clone]
[extern Threads_Exit]
[global SpawnTask]
SpawnTask:
	; Call Proc_Clone with Flags=0
	xor eax, eax
	push eax
	push eax
	call Proc_Clone
	add esp, 8	; Remove arguments from stack
	
	test eax, eax
	jnz .parent
	
	; In child, so now set up stack frame
	mov ebx, [esp+4]	; Child Function
	mov edx, [esp+8]	; Argument
	; Child
	push edx	; Argument
	call ebx	; Function
	call Threads_Exit	; Kill Thread
	
.parent:
	ret

;
; Calls a user fault handler
;
[global Proc_ReturnToUser]
[extern Proc_GetCurThread]
Proc_ReturnToUser:
	; EBP is the handler to use
	
	call Proc_GetCurThread
	
	; EAX is the current thread
	mov ebx, eax
	mov eax, [ebx+40]	; Get Kernel Stack
	sub eax, KSTACK_USERSTATE_SIZE	
	
	;
	; NOTE: This can cause corruption if the signal happens while the user
	;       has called a kernel operation.
	; Good thing this can only be called on a user fault.
	;
	
	; Get and alter User SP
	mov ecx, [eax+KSTACK_USERSTATE_SIZE-12]
	mov edx, [ebx+60]	; Get Signal Number
	mov [ecx-4], edx
	mov [ecx-8], DWORD User_Syscall_RetAndExit
	sub ecx, 8
	
	; Restore Segment Registers
	mov ax, 0x23
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	
	push 0x23	; SS
	push ecx	; ESP
	push 0x202	; EFLAGS (IP and Rsvd)
	push 0x1B	; CS
	push ebp	; EIP
	
	iret

[global GetCPUNum]
GetCPUNum:
	xor eax, eax
	ltr ax
	sub ax, 0x30
	shr ax, 3	; ax /= 8
	ret

[section .usertext]
User_Syscall_RetAndExit:
	push eax
	call User_Syscall_Exit
User_Syscall_Exit:
	xor eax, eax
	mov ebx, [esp+4]
	int 0xAC

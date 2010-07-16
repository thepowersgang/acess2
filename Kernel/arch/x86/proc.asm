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
	
	; Validate user ESP
	; - Page Table
	mov edx, [eax+KSTACK_USERSTATE_SIZE-12]	; User ESP is at top of kstack - 3*4
	%if USE_PAE
	%error PAE Support
	%else
	mov ecx, edx
	shr ecx, 22
	test BYTE [0xFC3F0000+ecx*4], 1
	jnz .justKillIt
	%endif
	; - Page
	mov ecx, edx
	shr ecx, 12
	test BYTE [0xFC000000+ecx*4], 1
	jnz .justKillIt
	; Adjust
	sub edx, 8
	; - Page Table
	%if USE_PAE
	%else
	mov ecx, edx
	shr ecx, 22
	test BYTE [0xFC3F0000+ecx*4], 1
	jnz .justKillIt
	%endif
	; - Page
	mov ecx, edx
	shr ecx, 12
	test BYTE [0xFC000000+ecx*4], 1
	jnz .justKillIt
	
	; Get and alter User SP
	mov ecx, edx
	mov edx, [ebx+60]	; Get Signal Number from TCB
	mov [ecx+4], edx	; Parameter (Signal/Error Number)
	mov [ecx], DWORD User_Syscall_RetAndExit	; Return Address
	
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
	
	; Just kill the bleeding thing
	; (I know it calls int 0xAC in kernel mode, but meh)
.justKillIt:
	xor eax, eax
	xor ebx, ebx
	dec ebx
	int 0xAC

[global GetCPUNum]
GetCPUNum:
	xor eax, eax
	str ax
	sub ax, 0x30
	shr ax, 3	; ax /= 8
	ret

; Usermode code exported by the kernel
[section .usertext]
User_Syscall_RetAndExit:
	push eax
	call User_Syscall_Exit
User_Syscall_Exit:
	xor eax, eax
	mov ebx, [esp+4]
	int 0xAC

; AcessOS Microkernel Version
; Start.asm

[bits 32]

KERNEL_BASE	equ 0xC0000000

KSTACK_USERSTATE_SIZE	equ	(4+8+1+5)*4	; SRegs, GPRegs, CPU, IRET

[section .text]

[global NewTaskHeader]
NewTaskHeader:
	mov eax, [esp]
	mov dr0, eax

	mov eax, [esp+4]
	add esp, 12	; Thread, Function, Arg Count
	call eax
	
	push eax	; Ret val
	push 0  	; 0 = This Thread
	call Threads_Exit

[extern MM_Clone]
[global Proc_CloneInt]
Proc_CloneInt:
	pusha
	; Save RSP
	mov eax, [esp+0x20+4]
	mov [eax], esp
	call MM_Clone
	; Save CR3
	mov esi, [esp+0x20+8]
	mov [esi], eax
	; Undo the pusha
	add esp, 0x20
	mov eax, .newTask
	ret
.newTask:
	popa
	xor eax, eax
	ret

[global SwitchTasks]
; + 4 = New RSP
; + 8 = Old RSP save loc
; +12 = New RIP
; +16 = Old RIP save loc
; +20 = CR3
SwitchTasks:
	pusha
	
	; Old IP
	mov eax, [esp+0x20+16]
	mov DWORD [eax], .restore
	; Old SP
	mov eax, [esp+0x20+8]
	mov [eax], esp

	mov ecx, [esp+0x20+12]	; New IP
	mov eax, [esp+0x20+20]	; New CR3
	mov esp, [esp+0x20+ 4]	; New SP
	test eax, eax
	jz .setState
	mov cr3, eax
	invlpg [esp]
	invlpg [esp+0x1000]
.setState:
;	xchg bx, bx
	jmp ecx

.restore:
	popa
	xor eax, eax
	ret


%if USE_MP
[extern giMP_TimerCount]
[extern gpMP_LocalAPIC]
[extern Isr240.jmp]
[global SetAPICTimerCount]
SetAPICTimerCount:
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
	
	mov eax, [gpMP_LocalAPIC]
	mov ecx, [eax+0x320]
	test ecx, 0x00010000
	jz .setTime
	mov DWORD [eax+0x380], 0xFFFFFFFF	; Set Initial Count
	mov DWORD [eax+0x320], 0x000000F0	; Enable the timer on IVT#0xEF (One Shot)
	jmp .ret

.setTime:	
	; Get Timer Count
	mov ecx, 0xFFFFFFFF
	sub ecx, [eax+0x390]
	mov DWORD [giMP_TimerCount], ecx
	; Disable APIC Timer
	mov DWORD [eax+0x320], 0x000100EF
	mov DWORD [eax+0x380], 0

	; Update Timer IRQ to the IRQ code
	mov eax, SchedulerBase
	sub eax, Isr240.jmp+5
	mov DWORD [Isr240.jmp+1], eax

	;xchg bx, bx	; MAGIC BREAK
.ret:
	mov dx, 0x20
	mov al, 0x20
	out dx, al		; ACK IRQ
	pop gs
	pop fs
	pop es
	pop ds
	popa
	add esp, 4	; CPU ID
	; No Error code / int num
	iret
%endif
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
	
	pushf
	and BYTE [esp+1], 0xFE	; Clear Trap Flag
	popf
	
	mov eax, dr0
	push eax	; Debug Register 0, Current Thread
	
	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	
	%if USE_MP
	call GetCPUNum
	mov ebx, eax
	push eax	; Push as argument
	%else
	push 0
	%endif
	
	call Proc_Scheduler
[global scheduler_return]
scheduler_return:	; Used by some hackery in Proc_DumpThreadCPUState
	
	add esp, 4	; Remove CPU Number (thread is poped later)

	%if USE_MP
	test ebx, ebx
	jnz .sendEOI
	%endif
	
	mov al, 0x20
	out 0x20, al		; ACK IRQ
	%if USE_MP
	jmp .ret
	
.sendEOI:
	mov eax, DWORD [gpMP_LocalAPIC]
	mov DWORD [eax+0x0B0], 0
	%endif
.ret:
	pop eax	; Debug Register 0, Current Thread
	mov dr0, eax

	pop gs
	pop fs
	pop es
	pop ds
	
	popa
	add esp, 4*2	; CPU ID + Dummy error code
	; No Error code / int num
	iret

[extern Proc_Clone]
[extern Threads_Exit]
[global SpawnTask]
SpawnTask:
	; Call Proc_Clone with Flags=0
	xor eax, eax
;	push eax
	push eax
	call Proc_Clone
	add esp, 8	; Remove arguments from stack
	
	test eax, eax
	jnz .parent
	
	; In child, so now set up stack frame
	mov ebx, [esp+4]	; Child Function
	mov edx, [esp+8]	; Argument
	; Child Function
	push edx	; Argument
	call ebx	; Function
	; Kill thread once done
	push eax	; Exit Code
	push   0	; Kill this thread
	call Threads_Exit	; Kill Thread
	
.parent:
	ret

; void Proc_ReturnToUser(void *Method, Uint Parameter)
; Calls a user fault handler
;
[global Proc_ReturnToUser]
[extern Proc_GetCurThread]
Proc_ReturnToUser:
	push ebp
	mov ebp, esp
	; [EBP+8]: handler to use
	; [EBP+12]: parameter
	; [EBP+16]: kernel stack top
	
	;call Proc_GetCurThread
	
	; EAX is the current thread
	;mov ebx, eax
	;mov eax, [ebx+12*4]	; Get Kernel Stack
	mov eax, [ebp+16]	; Get Kernel Stack
	sub eax, KSTACK_USERSTATE_SIZE
	
	;
	; NOTE: This can cause corruption if the signal happens while the user
	;       has called a kernel operation.
	; Good thing this can only be called on a user fault.
	;
	
	; Validate user ESP
	; - Page Table
	mov edx, [eax+KSTACK_USERSTATE_SIZE-12]	; User ESP is at top of kstack - 3*4
	mov ecx, edx
	shr ecx, 22
	test BYTE [0xFC3F0000+ecx*4], 1
	jnz .justKillIt
	; - Page
	mov ecx, edx
	shr ecx, 12
	test BYTE [0xFC000000+ecx*4], 1
	jnz .justKillIt
	; Adjust
	sub edx, 8
	; - Page Table
	mov ecx, edx
	shr ecx, 22
	test BYTE [0xFC3F0000+ecx*4], 1
	jnz .justKillIt
	; - Page
	mov ecx, edx
	shr ecx, 12
	test BYTE [0xFC000000+ecx*4], 1
	jnz .justKillIt
	
	; Get and alter User SP
	mov edi, edx
	mov edx, [ebp+12]	; Get parameter
	mov [edi+4], edx	; save to user stack
	mov [edi], DWORD User_Syscall_RetAndExit	; Return Address
	
	; Restore Segment Registers
	mov ax, 0x23
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	
	push 0x23	; SS
	push edi	; ESP
	push 0x202	; EFLAGS (IP and Rsvd)
	push 0x1B	; CS
	mov eax, [ebp+8]	; Method to call
	push eax	; EIP
	
	iret
	
	; Just kill the bleeding thing
	; (I know it calls int 0xAC in kernel mode, but meh)
.justKillIt:
	xor eax, eax
	xor ebx, ebx
	dec ebx	; EBX = -1
	int 0xAC

[global GetCPUNum]
GetCPUNum:	; TODO: Store in debug registers
;	xor eax, eax
;	str ax
;	sub ax, 0x30
;	shr ax, 3	; ax /= 8
	mov eax, dr1
	ret

[extern GetEIP]
[global GetEIP_Sched]
[global GetEIP_Sched_ret]
GetEIP_Sched_ret equ GetEIP_Sched.ret
GetEIP_Sched:
	call GetEIP
GetEIP_Sched.ret:
	ret

; Usermode code exported by the kernel
[section .usertext]
; Export a place for the user to jump to to call a syscall
; - Allows the kernel to change the method easily
User_Syscall:
	xchg bx, bx	; MAGIC BREAKPOINT
	int 0xAC

; A place to return to and exit
User_Syscall_RetAndExit:
	push eax
	call User_Syscall_Exit
User_Syscall_Exit:
	xor eax, eax
	mov ebx, [esp+4]
	int 0xAC

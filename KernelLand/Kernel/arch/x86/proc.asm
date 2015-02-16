; AcessOS Microkernel Version
; Start.asm
%include "arch/x86/common.inc.asm"

[bits 32]

%define SAVEFLAG_FPU	0x1

KERNEL_BASE	equ 0xC0000000

KSTACK_USERSTATE_SIZE	equ	(4+8+1+5)*4	; SRegs, GPRegs, CPU, IRET

[section .text]

[extern MM_Clone]
[global Proc_CloneInt]
Proc_CloneInt:
	pusha
	; Save RSP
	mov eax, [esp+0x20+4]
	mov [eax], esp
	push DWORD [esp+0x20+12]
	call MM_Clone
	add esp, 4
	; Save CR3
	mov esi, [esp+0x20+8]
	mov [esi], eax
	; Undo the pusha
	popa
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
	PUSH_CC
	
	; Old IP
	mov eax, [esp+0x20+16]
	test eax, eax
	jz .nosave
	mov DWORD [eax], .restore
	; Old SP
	mov eax, [esp+0x20+8]
	mov [eax], esp

.nosave:
	mov ecx, [esp+0x20+12]	; New IP
	mov eax, [esp+0x20+20]	; New CR3
	mov esp, [esp+0x20+ 4]	; New SP
	
	test eax, eax
	jz .setState
	mov cr3, eax
	invlpg [esp]
	invlpg [esp+0x1000]
.setState:
	jmp ecx

.restore:
	POP_CC
	xor eax, eax
	ret

[global Proc_InitialiseSSE]
Proc_InitialiseSSE:
	mov eax, cr4
	or eax, (1 << 9)|(1 << 10)	; Set OSFXSR and OSXMMEXCPT
	mov cr4, eax
	mov eax, cr0
	and ax, ~(1 << 2)	; Clear EM
	or eax, (1 << 1)	; Set MP
	mov eax, cr0
	ret
[global Proc_DisableSSE]
Proc_DisableSSE:
	mov eax, cr0
	or ax, 1 << 3	; Set TS
	mov cr0, eax
	ret
[global Proc_EnableSSE]
Proc_EnableSSE:
	mov eax, cr0
	and ax, ~(1 << 3)	; Clear TS
	mov cr0, eax
	ret

[global Proc_SaveSSE]
Proc_SaveSSE:
	mov eax, [esp+4]
	fxsave [eax]
	ret
[global Proc_RestoreSSE]
Proc_RestoreSSE:
	mov eax, [esp+4]
	fxrstor [eax]
	ret

%if USE_MP
[extern giMP_TimerCount]
[extern gpMP_LocalAPIC]
[extern Isr240.jmp]
[global SetAPICTimerCount]
SetAPICTimerCount:
	PUSH_CC
	PUSH_SEG
	
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
	mov eax, Proc_EventTimer_PIT
	sub eax, Isr240.jmp+5
	mov DWORD [Isr240.jmp+1], eax

	;xchg bx, bx	; MAGIC BREAK
.ret:
	mov dx, 0x20
	mov al, 0x20
	out 0x20, al		; ACK IRQ
	POP_SEG
	POP_CC
	add esp, 8	; CPU ID / Error Code
	iret
%endif

%if USE_MP
[global Proc_EventTimer_LAPIC]
Proc_EventTimer_LAPIC:
	push eax
	mov eax, SS:[gpMP_LocalAPIC]
	mov DWORD SS:[eax + 0xB0], 0
	pop eax
	jmp Proc_EventTimer_Common
%endif
[global Proc_EventTimer_PIT]
Proc_EventTimer_PIT:
	push eax
	mov al, 0x20
	out 0x20, al		; ACK IRQ
	pop eax
	jmp Proc_EventTimer_Common
[extern Proc_HandleEventTimer]
[global Proc_EventTimer_Common]
Proc_EventTimer_Common:
	PUSH_CC
	PUSH_SEG
	
	; Clear the Trace/Trap flag
	pushf
	and BYTE [esp+1], 0xFE	; Clear Trap Flag
	popf
	; Re-enable interrupts
	; - TODO: This is quite likely racy, if we get an interrupt flood
	sti
	
	%if USE_MP
	call GetCPUNum
	push eax	; Push as argument
	%else
	push 0
	%endif
	
	call Proc_HandleEventTimer
[global scheduler_return]
scheduler_return:	; Used by some hackery in Proc_DumpThreadCPUState
	add esp, 4	; Remove CPU Number (thread is poped later)

	jmp ReturnFromInterrupt

;
; Returns after an interrupt, restoring the user state
; - Also handles signal handlers
;
[global ReturnFromInterrupt]
[extern Threads_GetPendingSignal]
[extern Threads_GetSignalHandler]
[extern Proc_CallUser]
ReturnFromInterrupt:
	; Check that we're returning to userland
	test DWORD [esp+(4+8+2+1)*4], 0x07
	jz .ret	; Kernel interrupted, return

	call Threads_GetPendingSignal
	; eax: signal number
	test eax, eax
	jz .ret
	
	; There's a signal pending, funtime
	push eax
	call Threads_GetSignalHandler
	; eax: signal handler
	pop ecx
	test eax, eax
	jz .ret
	cmp eax, -1
	jz .default
	
	; (some stack abuse)
	push User_RestoreState
	push ecx
	mov ecx, esp
	
	push (2+4+8+2+2)*4	; Up to and incl. CS
	push ecx	; User stack data base
	push DWORD [ecx+(2+4+8+2+3)*4]	; User SP
	push eax	; handler
	call Proc_CallUser
	; Oh, ... it failed. Default time?
	add esp, (4+2)*4
.default:
	

	; Fall through to return
.ret:
	pop gs
	pop fs
	pop es
	pop ds
	popa
	add esp, 2*4	; IRQ Num / CPU ID + error code
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

; void Proc_ReturnToUser(void *Method, Uint Parameter, tVAddr KernelStack)
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
	
	; Get kernel stack	
	mov eax, [ebp+16]
	sub eax, KSTACK_USERSTATE_SIZE
	
	;
	; NOTE: This can cause corruption if the signal happens while the user
	;       has called a kernel operation.
	; Good thing this can only be called on a user fault.
	;
	
	; Create data to add to user stack
	push DWORD [ebp+12]
	push User_Syscall_RetAndExit
	mov ecx, esp

	; Call user method	
	push 2*4
	push ecx
	push DWORD [eax+KSTACK_USERSTATE_SIZE-12]	; User ESP is at top of kstack - 3*4
	push DWORD [ebp+8]
	call Proc_CallUser	
	
	; Just kill the bleeding thing
	; (I know it calls int 0xAC in kernel mode, but meh)
.justKillIt:
	xor eax, eax
	xor ebx, ebx
	dec ebx	; EBX = -1
	int 0xAC

[global GetCPUNum]
GetCPUNum:	; TODO: Store in debug registers
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

[global User_Signal_Kill]
User_Signal_Kill:
	xor eax, eax
	mov bl, [esp+4]
	mov bh, 0x02
	int 0xAC
	jmp $

User_RestoreState:
	pop gs
	pop fs
	pop es
	pop ds
	popa
	add esp, 2*4	; Kernel's error code and interrupt number
	retf	; EFLAGS/SS/ESP were not included in the state
	

; A place to return to and exit
User_Syscall_RetAndExit:
	push eax
	call User_Syscall_Exit
User_Syscall_Exit:
	xor eax, eax
	mov ebx, [esp+4]
	int 0xAC

; vim: ft=nasm ts=8

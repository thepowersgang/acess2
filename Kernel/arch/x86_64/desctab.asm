;
;
;
%include "arch/x86_64/include/common.inc.asm"
[BITS 64]

[extern Log]
[extern gGDTPtr]
[extern gGDT]

%define NUM_IRQ_CALLBACKS	4

MM_LOCALAPIC	equ	0xFFFFFD0000000000

[section .text]
[global Desctab_Init]
Desctab_Init:	
	; Save to make following instructions smaller
	mov rdi, gIDT
	
	; Set an IDT entry to a callback
	%macro SETIDT 2
	mov rax, %2
	mov WORD [rdi + %1*16], ax
	shr rax, 16
	mov WORD [rdi + %1*16 + 6], ax
	shr rax, 16
	mov DWORD [rdi + %1*16 + 8], eax
	; Enable
	mov ax, WORD [rdi + %1*16 + 4]
	or  ax, 0x8000
	mov WORD [rdi + %1*16 + 4], ax
	%endmacro
	
	; Install error handlers
	%macro SETISR 1
	SETIDT %1, Isr%1
	%endmacro
	
	%assign i 0
	%rep 32
	SETISR i
	%assign i i+1
	%endrep
	
	; Install IRQs
	SETIDT	0xF0, PIT_IRQ
	SETIDT	0xF1, Irq1
	SETIDT	0xF2, Irq2
	SETIDT	0xF3, Irq3
	SETIDT	0xF4, Irq4
	SETIDT	0xF5, Irq5
	SETIDT	0xF6, Irq6
	SETIDT	0xF7, Irq7
	SETIDT	0xF8, Irq8
	SETIDT	0xF9, Irq9
	SETIDT	0xFA, Irq10
	SETIDT	0xFB, Irq11
	SETIDT	0xFC, Irq12
	SETIDT	0xFD, Irq13
	SETIDT	0xFE, Irq14
	SETIDT	0xFF, Irq15

	; Remap PIC
	push rdx	; Save RDX
	mov dx, 0x20
	mov al, 0x11
	out dx, al	;	Init Command
	mov dx, 0x21
	mov al, 0xF0
	out dx, al	;	Offset (Start of IDT Range)
	mov al, 0x04
	out dx, al	;	IRQ connected to Slave (00000100b) = IRQ2
	mov al, 0x01
	out dx, al	;	Set Mode
	mov al, 0x00
	out dx, al	;	Set Mode
	
	mov dx, 0xA0
	mov al, 0x11
	out dx, al	;	Init Command
	mov dx, 0xA1
	mov al, 0xF8
	out dx, al	;	Offset (Start of IDT Range)
	mov al, 0x02
	out dx, al	;	IRQ Line connected to master
	mov al, 0x01
	out dx, al	;	Set Mode
	mov dl, 0x00
	out dx, al	;	Set Mode
	pop rdx
	
	
	; Install IDT
	mov rax, gIDTPtr
	lidt [rax]
	
	; Re-install GDT (in higher address space)
	mov rax, gGDTPtr
	mov rcx, gGDT
	mov QWORD [rax+2], rcx
	lgdt [rax]
	
	; Start interrupts
	sti

	; Initialise System Calls (SYSCALL/SYSRET)
	; Set IA32_EFER.SCE
	mov ecx, 0xC0000080
	rdmsr
	or eax, 1
	wrmsr
	; Set IA32_LSTAR (RIP of handler)
	mov ecx, 0xC0000082	; IA32_LSTAR
	mov eax, SyscallStub - 0xFFFFFFFF00000000
	mov edx, 0xFFFFFFFF
	wrmsr
	; Set IA32_FMASK (flags mask)
	mov ecx, 0xC0000084
	rdmsr
	mov eax, ~0x202
	wrmsr
	; Set IA32_STAR (Kernel/User CS)
	mov ecx, 0xC0000081
	rdmsr
	mov edx, 0x8 | (0x1B << 16)	; Kernel CS (and Kernel DS/SS - 8), User CS
	wrmsr
	
	ret

; int IRQ_AddHandler(int IRQ, void (*Handler)(int IRQ), void *Ptr)
; Return Values:
;  0 on Success
; -1 on an invalid IRQ Number
; -2 when no slots are avaliable
[global IRQ_AddHandler]
IRQ_AddHandler:
	; RDI - IRQ Number
	; RSI - Callback
	; RDX - Ptr
	
	; Check for RDI >= 16
	cmp rdi, 16
	jb .numOK
	xor rax, rax
	dec rax
	jmp .ret
.numOK:

	; Get handler base into RAX
	lea rax, [rdi*4]
	mov rcx, gaIRQ_Handlers
	lea rax, [rcx+rax*8]
	
	; Find a free callback slot
	%rep NUM_IRQ_CALLBACKS
	mov rcx, [rax]
	test rcx, rcx
	jz .assign
	add rax, 8
	%endrep
	; None found, return -2
	xor rax, rax
	dec rax
	dec rax
	jmp .ret
	
	; Assign the IRQ Callback
.assign:
	; A little bit of debug
	push rdi
	push rsi
	push rax
	push rdx
	sub rsp, 8
	mov rcx, rdi	; IRQ Number
	mov rdx, rsi	; Callback
	mov rsi, rax	; Pointer
	mov rdi, csIRQ_Assigned
	call Log
	add rsp, 8
	pop rdx
	pop rax
	pop rsi
	pop rdi

	; Assign and return
	mov [rax], rsi
	add rax, gaIRQ_DataPtrs - gaIRQ_Handlers
	mov [rax], rdx
	xor rax, rax

.ret:
	ret
	
[section .rodata]
csIRQ_Assigned:
	db	"IRQ %p := %p (IRQ %i)",0
csIRQ_Fired:
	db	"IRQ %i fired",0
[section .text]

%macro ISR_NOERRNO	1
Isr%1:
	push	QWORD 0
	push	QWORD %1
	jmp	ErrorCommon
%endmacro
%macro ISR_ERRNO	1
Isr%1:
	push	QWORD %1
	jmp	ErrorCommon
%endmacro

ISR_NOERRNO	0;  0: Divide By Zero Exception
ISR_NOERRNO	1;  1: Debug Exception
ISR_NOERRNO	2;  2: Non Maskable Interrupt Exception
ISR_NOERRNO	3;  3: Int 3 Exception
ISR_NOERRNO	4;  4: INTO Exception
ISR_NOERRNO	5;  5: Out of Bounds Exception
ISR_NOERRNO	6;  6: Invalid Opcode Exception
ISR_NOERRNO	7;  7: Coprocessor Not Available Exception
ISR_ERRNO	8;  8: Double Fault Exception (With Error Code!)
ISR_NOERRNO	9;  9: Coprocessor Segment Overrun Exception
ISR_ERRNO	10; 10: Bad TSS Exception (With Error Code!)
ISR_ERRNO	11; 11: Segment Not Present Exception (With Error Code!)
ISR_ERRNO	12; 12: Stack Fault Exception (With Error Code!)
ISR_ERRNO	13; 13: General Protection Fault Exception (With Error Code!)
ISR_ERRNO	14; 14: Page Fault Exception (With Error Code!)
ISR_NOERRNO	15; 15: Reserved Exception
ISR_NOERRNO	16; 16: Floating Point Exception
ISR_NOERRNO	17; 17: Alignment Check Exception
ISR_NOERRNO	18; 18: Machine Check Exception
ISR_NOERRNO	19; 19: Reserved
ISR_NOERRNO	20; 20: Reserved
ISR_NOERRNO	21; 21: Reserved
ISR_NOERRNO	22; 22: Reserved
ISR_NOERRNO	23; 23: Reserved
ISR_NOERRNO	24; 24: Reserved
ISR_NOERRNO	25; 25: Reserved
ISR_NOERRNO	26; 26: Reserved
ISR_NOERRNO	27; 27: Reserved
ISR_NOERRNO	28; 28: Reserved
ISR_NOERRNO	29; 29: Reserved
ISR_NOERRNO	30; 30: Reserved
ISR_NOERRNO	31; 31: Reserved

[extern Error_Handler]
[global ErrorCommon]
ErrorCommon:
	PUSH_GPR
	push gs
	push fs
	;PUSH_FPU
	;PUSH_XMM
	
	mov rdi, rsp
;	xchg bx, bx
	call Error_Handler
	
	;POP_XMM
	;POP_FPU
	pop fs
	pop gs
	POP_GPR
	add rsp, 2*8
	iretq

%macro DEFIRQ	1
Irq%1:
	push	0
	push	%1
	jmp	IrqCommon
%endmacro

%assign i 0
%rep 16
DEFIRQ	i
%assign i i+1
%endrep

[global IrqCommon]
IrqCommon:
	PUSH_GPR
	push gs
	push fs

;	mov rdi, csIRQ_Fired
;	mov rsi, [rsp+(16+2)*8]
;	call Log
	
	mov ebx, [rsp+(16+2)*8]	; Get interrupt number (16 GPRS + 2 SRs)
	shl ebx, 2	; *4
	mov rax, gaIRQ_Handlers
	lea rbx, [rax+rbx*8]
	
	; Check all callbacks
	sub rsp, 8	; Shadow of argument
	%assign i 0
	%rep NUM_IRQ_CALLBACKS
	; Get callback address
	mov rax, [rbx]
	test rax, rax	; Check if it exists
	jz .skip.%[i]
	; Set RDI to IRQ number
	mov rdi, [rsp+(16+2+1)*8]	; Get IRQ number
	mov rsi, [rbx-gaIRQ_Handlers+gaIRQ_DataPtrs]
	call rax	; Call
.skip.%[i]:
	add rbx, 8	; Next!
	%assign i i+1
	%endrep
	add rsp, 8
	
	; ACK
	mov al, 0x20
	mov rdi, [rsp+(16+2)*8]	; Get IRQ number
	cmp rdi, 8
	jb .skipAckSecondary
	out 0xA0, al
.skipAckSecondary:
	out 0x20, al
	
	pop fs
	pop gs
	POP_GPR
	add rsp, 8*2
	iretq

[extern Time_UpdateTimestamp]

%if USE_MP
[global APIC_Timer_IRQ]
APIC_Timer_IRQ:
	PUSH_GPR
	push gs
	push fs

	; TODO: What to do?

	mov eax, DWORD [gpMP_LocalAPIC]
	mov DWORD [eax+0x0B0], 0

	pop fs
	pop gs
	POP_GPR
	iretq
%endif

[global PIT_IRQ]
PIT_IRQ:
	PUSH_GPR
	;PUSH_FPU
	;PUSH_XMM
	
	call Time_UpdateTimestamp

	%if 0
[section .rodata]
csUserSS:	db	"User SS: 0x%x",0
[section .text]
	mov rdi, csUserSS
	mov rsi, [rsp+0x80+0x20]
	call Log
	%endif

	; Send EOI
	mov al, 0x20
	out 0x20, al		; ACK IRQ
	
	;POP_XMM
	;POP_FPU
	POP_GPR
	iretq

[extern ci_offsetof_tThread_KernelStack]
[extern SyscallHandler]
[global SyscallStub]
SyscallStub:
	mov rbp, dr0
	mov ebx, [rel ci_offsetof_tThread_KernelStack]
	mov rbp, [rbp+rbx]	; Get kernel stack
	xchg rbp, rsp	; Swap stacks

	push rbp	; Save User RSP
	push rcx	; RIP
	push r11	; RFLAGS

	; RDI
	; RSI
	; RDX
	; R10 (RCX for non syscall)
	; R8
	; R9
	sub rsp, (6+2)*8
	mov [rsp+0x00], rax	; Number
;	mov [rsp+0x08], rax	; Errno (don't care really)
	mov [rsp+0x10], rdi	; Arg1
	mov [rsp+0x18], rsi	; Arg2
	mov [rsp+0x20], rdx	; Arg3
	mov [rsp+0x28], r10	; Arg4
	mov [rsp+0x30], r8	; Arg5
	mov [rsp+0x38], r9	; Arg6
	
	mov rdi, rsp
	sub rsp, 8
	call SyscallHandler

	%if 0
[section .rodata]
csSyscallReturn:	db	"Syscall Return: 0x%x",0
[section .text]
	mov rdi, csSyscallReturn
	mov rsi, [rsp+0+8]
	call Log
	%endif

	add rsp, 8
	mov ebx, [rsp+8]	; Get errno
	mov rax, [rsp+0]	; Get return
	add rsp, (6+2)*8

	pop r11
	pop rcx
	pop rsp 	; Change back to user stack
	; TODO: Determine if user is 64 or 32 bit

	db 0x48	; REX, nasm doesn't have a sysretq opcode
	sysret

[section .data]
gIDT:
	; 64-bit Interrupt Gate, CS = 0x8, IST0 (Disabled)
	times 256	dd	0x00080000, 0x00000E00, 0, 0
gIDTPtr:
	dw	256*16-1
	dq	gIDT

gaIRQ_Handlers:
	times	16*NUM_IRQ_CALLBACKS	dq	0
gaIRQ_DataPtrs:
	times	16*NUM_IRQ_CALLBACKS	dq	0

; vim: ft=nasm

;
;
;
[BITS 64]

MM_LOCALAPIC	equ	0xFFFFFD0000000000

[section .text]
Desctab_Init:
	; Install IRQ Handlers

[section .data]
gIDT:
	times 256	dw	0x00080000, 0x00008E00, 0, 0	; 64-bit Interrupt Gate, CS = 0x8, IST0

%macro DEFERR	1
Isr%1:
	push	0
	push	%1
	jmp	ErrorCommon
%endmacro
%macro DEFERRNO	1
Isr%1:
	push	%1
	jmp	ErrorCommon
%endmacro

%macro DEFIRQ	1
Irq%1:
	push	0
	push	%1
	jmp	IrqCommon
%endmacro

%macro PUSH_EX	1-*
	%rep %0
	push %1
	%rotate 1
	%endrep
%endmacro
%macro POP_EX	1-*
	%rep %0
	%rotate -1
	pop %1
	%endrep
%endmacro

%macro PUSH_GPR	0
	PUSH_EX	r15, r14, r13, r12, r11, r10, r9, r8
	PUSH_EX	rsi, rdi, rbp, rsp, rbx, rdx, rcx, rax
%endmacro
%macro POP_GPR	0
	POP_EX	rsi, rdi, rbp, rsp, rbx, rdx, rcx, rax
	POP_EX	r15, r14, r13, r12, r11, r10, r9, r8
%endmacro

[extern Proc_Scheduler]
SchedulerIRQ:
	; TODO: Find Current CPU
	PUSH_GPR
	;PUSH_FPU
	;PUSH_XMM
	
	xor rsi, rsi
	mov rdi, MM_LOCALAPIC+0x20
	mov esi, [rdi]
	call Proc_Scheduler
	
	;POP_XMM
	;POP_FPU
	POP_GPR
	iret

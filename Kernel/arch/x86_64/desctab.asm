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
	mov [rsp-0x08], r15
	mov [rsp-0x10], r14
	mov [rsp-0x18], r13
	mov [rsp-0x20], r12
	mov [rsp-0x28], r11
	mov [rsp-0x30], r10
	mov [rsp-0x38], r9
	mov [rsp-0x40], r8
	mov [rsp-0x48], rdi
	mov [rsp-0x50], rsi
	mov [rsp-0x58], rbp
	mov [rsp-0x60], rsp
	mov [rsp-0x68], rbx
	mov [rsp-0x70], rdx
	mov [rsp-0x78], rcx
	mov [rsp-0x80], rax
	sub rsp, 0x80
%endmacro
%macro POP_GPR	0
	add rsp, 0x80
	mov r15, [rsp-0x08]
	mov r14, [rsp-0x10]
	mov r13, [rsp-0x18]
	mov r12, [rsp-0x20]
	mov r11, [rsp-0x28]
	mov r10, [rsp-0x30]
	mov r9, [rsp-0x38]
	mov r8, [rsp-0x40]
	mov rdi, [rsp-0x48]
	mov rsi, [rsp-0x50]
	mov rbp, [rsp-0x58]
	;mov rsp, [rsp-0x60]
	mov rbx, [rsp-0x68]
	mov rdx, [rsp-0x70]
	mov rcx, [rsp-0x78]
	mov rax, [rsp-0x80]
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

;
;
;
[BITS 64]

%define NUM_IRQ_CALLBACKS	4

MM_LOCALAPIC	equ	0xFFFFFD0000000000

%macro PUSH_GPR	0
	mov [rsp-0x60], rsp
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
	mov r9,  [rsp-0x38]
	mov r8,  [rsp-0x40]
	mov rdi, [rsp-0x48]
	mov rsi, [rsp-0x50]
	mov rbp, [rsp-0x58]
	;mov rsp, [rsp-0x60]
	mov rbx, [rsp-0x68]
	mov rdx, [rsp-0x70]
	mov rcx, [rsp-0x78]
	mov rax, [rsp-0x80]
%endmacro

[section .text]
Desctab_Init:
	; Install IRQ Handlers
	ret

; int IRQ_AddHandler(int IRQ, void (*Handler)(int IRQ))
; Return Values:
;  0 on Success
; -1 on an invalid IRQ Number
; -2 when no slots are avaliable
[global IRQ_AddHandler]
IRQ_AddHandler:
	; RDI - IRQ Number
	; RSI - Callback
	
	cmp rdi, 16
	jb .numOK
	xor rax, rax
	dec rax
	jmp .ret
.numOK:

	mov rax, rdi
	shr rax, 3+2
	mov rcx, gaIRQ_Handlers
	add rax, rcx
	
	; Find a free callback slot
	%rep NUM_IRQ_CALLBACKS
	mov rdx, [rax]
	test rdx, rdx
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
	mov [rax], rsi
	xor rax, rax

.ret:
	ret

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

IrqCommon:
	PUSH_GPR
	
	mov rbx, [rsp+16*8]	; Calculate address
	shr rbx, 3+2	; *8*4
	mov rax, gaIRQ_Handlers
	add rbx, rax
	
	%assign i 0
	%rep NUM_IRQ_CALLBACKS
	mov rax, [rbx]
	test rax, rax
	mov rdi, [rsp+16*8]	; Get IRQ number
	jz .skip.%[i]
	call rax	; 2 Bytes (Op and Mod/RM)
.skip.%[i]:
	add rbx, 8
	%assign i i+1
	%endrep
	
	mov rdi, [rsp+16*8]	; Get IRQ number
	cmp rdi, 8
	mov al, 0x20
	jb .skipAckSecondary
	mov dx, 0x00A0
	out dx, al
.skipAckSecondary:
	mov dx, 0x0020
	out dx, al
	
	POP_GPR
	add rsp, 16
	iret

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

[section .data]
gIDT:
	times 256	dw	0x00080000, 0x00008E00, 0, 0	; 64-bit Interrupt Gate, CS = 0x8, IST0

gaIRQ_Handlers:
	times	16*NUM_IRQ_CALLBACKS	dq	0

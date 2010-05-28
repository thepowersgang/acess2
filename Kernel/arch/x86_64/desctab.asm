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
[global Desctab_Init]
Desctab_Init:	
	; Save to make following instructions smaller
	mov rdi, gIDT
	
	; Set an IDT entry to a callback
	%macro SETIDT 2
	mov rax, %2
	mov	WORD [rdi + %1*16], ax
	shr rax, 16
	mov	WORD [rdi + %1*16 + 6], ax
	shr rax, 16
	mov DWORD [rdi + %1*16 + 8], eax
	; Enable
	mov	ax, WORD [rdi + %1*16 + 4]
	or ax, 0x8000
	mov	WORD [rdi + %1*16 + 4], ax
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
	SETIDT	0xF0, Irq0
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
	
	; Start interrupts
	sti
	
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
	xchg bx, bx
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
	
	mov rbx, [rsp+16*8]	; Calculate address
	shr rbx, 3+2	; *8*4
	mov rax, gaIRQ_Handlers
	add rbx, rax
	
	; Check all callbacks
	%assign i 0
	%rep NUM_IRQ_CALLBACKS
	; Get callback address
	mov rax, [rbx]
	test rax, rax	; Check if it exists
	jz .skip.%[i]
	; Set RDI to IRQ number
	mov rdi, [rsp+16*8]	; Get IRQ number
	call rax	; Call
.skip.%[i]:
	add rbx, 8	; Next!
	%assign i i+1
	%endrep
	
	; ACK
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
	add rsp, 8*2
	xchg bx, bx
	iretq

[extern Proc_Scheduler]
[global SchedulerIRQ]
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
	add rsp, 8*2
	iretq

[section .data]
gIDT:
	; 64-bit Interrupt Gate, CS = 0x8, IST0 (Disabled)
	times 256	dd	0x00080000, 0x00000E00, 0, 0
gIDTPtr:
	dw	256*16-1
	dq	gIDT

gaIRQ_Handlers:
	times	16*NUM_IRQ_CALLBACKS	dq	0

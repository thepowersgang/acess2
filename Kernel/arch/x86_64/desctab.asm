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
	; Install IDT
	mov rax, gIDTPtr
	lidt [rax]
	
	; Save to make following instructions smaller
	mov rdi, gIDT
	
	; Set an IDT entry to a callback
	%macro SETIDT 2
	mov rax, %2
	mov	WORD [rdi + %1*16], ax
	shr rax, 16
	mov	WORD [rdi + %1*16+6], ax
	shr rax, 16
	mov DWORD [rdi+%1*16+8], eax
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
	%macro SETIRQ 2
	SETIDT %2, Irq%1
	%endmacro
	
	%assign i 0
	%rep 16
	SETIRQ i, 0xF0+i
	%assign i i+1
	%endrep
	
	; Remap PIC
	
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
	push	0
	push	%1
	jmp	ErrorCommon
%endmacro
%macro ISR_ERRNO	1
Isr%1:
	push	%1
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

ErrorCommon:
	add rsp, 2*8
	iret

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
	times 256	dd	0x00080000, 0x00008E00, 0, 0	; 64-bit Interrupt Gate, CS = 0x8, IST0
gIDTPtr:
	dw	256*16-1
	dq	gIDT

gaIRQ_Handlers:
	times	16*NUM_IRQ_CALLBACKS	dq	0

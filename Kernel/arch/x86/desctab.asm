; AcessOS Microkernel Version
;
; desctab.asm
[BITS 32]

%if ARCH == "i386"
MAX_CPUS	equ	1
%else
MAX_CPUS	equ	8
%endif
GDT_SIZE	equ	(1+2*2+1+MAX_CPUS)*8

[section .data]
; GDT
[global gGDT]
gGDT:
	; PL0 - Kernel
	; PL3 - User
	dd 0x00000000, 0x00000000	; 00 NULL Entry
	dd 0x0000FFFF, 0x00CF9A00	; 08 PL0 Code
	dd 0x0000FFFF, 0x00CF9200	; 10 PL0 Data
	dd 0x0000FFFF, 0x00CFFA00	; 18 PL3 Code
	dd 0x0000FFFF, 0x00CFF200	; 20 PL3 Data
	dd 26*4-1, 0x00408900	; Double Fault TSS
	times MAX_CPUS	dd 26*4-1, 0x00408900
gGDTptr:
	dw	GDT_SIZE-1
	dd	gGDT
; IDT
ALIGN 8
gIDT:
	times	256	dd	0x00080000,0x00000F00
gIDTPtr:
	dw	256 * 16 - 1	; Limit
	dd	gIDT		; Base

[section .text]
[global Desctab_Install]
Desctab_Install:
	; Set GDT
	lgdt [gGDTptr]
	mov ax, 0x10	; PL0 Data
	mov ss, ax
	mov ds, ax
	mov es, ax
	mov gs, ax
	mov fs, ax
	jmp 0x08:.pl0code
.pl0code:

	; Set IDT
%macro SETISR	1
	mov eax, Isr%1
	mov	WORD [gIDT + %1*8], ax
	shr eax, 16
	mov	WORD [gIDT + %1*8+6], ax
	mov	ax, WORD [gIDT + %1*8 + 4]
	or ax, 0x8000
	mov	WORD [gIDT + %1*8 + 4], ax
%endmacro
%macro SETUSER	1
	mov	ax, WORD [gIDT + %1*8 + 4]
	or ax, 0x6000
	mov	WORD [gIDT + %1*8 + 4], ax
%endmacro
	%assign	i	0
	%rep 32
	SETISR	i
	%assign i i+1
	%endrep
	
	SETISR	0xAC
	SETUSER	0xAC
	
	%assign	i	0xF0
	%rep 16
	SETISR	i
	%assign i i+1
	%endrep

	; Load IDT
	lidt [gIDTPtr]

	; Remap PIC
	push edx	; Save EDX
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
	pop edx
	
	ret


; ===============
; = Define ISRs =
; ===============
%macro	ISR_ERRNO	1
[global Isr%1]
Isr%1:
	;xchg bx, bx
	push	%1
	jmp	ErrorCommon
%endmacro
%macro	ISR_NOERR	1
[global Isr%1]
Isr%1:
	xchg bx, bx
	push	0
	push	%1
	jmp	ErrorCommon
%endmacro

%macro DEF_SYSCALL	1
[global Isr%1]
Isr%1:
	push	0
	push	%1
	jmp	SyscallCommon
%endmacro

%macro DEF_IRQ	1
[global Isr%1]
Isr%1:
	;cli	; HACK!
	push	0
	push	%1
	jmp	IRQCommon
%endmacro

ISR_NOERR	0;  0: Divide By Zero Exception
ISR_NOERR	1;  1: Debug Exception
ISR_NOERR	2;  2: Non Maskable Interrupt Exception
ISR_NOERR	3;  3: Int 3 Exception
ISR_NOERR	4;  4: INTO Exception
ISR_NOERR	5;  5: Out of Bounds Exception
ISR_NOERR	6;  6: Invalid Opcode Exception
ISR_NOERR	7;  7: Coprocessor Not Available Exception
ISR_ERRNO	8;  8: Double Fault Exception (With Error Code!)
ISR_NOERR	9;  9: Coprocessor Segment Overrun Exception
ISR_ERRNO	10; 10: Bad TSS Exception (With Error Code!)
ISR_ERRNO	11; 11: Segment Not Present Exception (With Error Code!)
ISR_ERRNO	12; 12: Stack Fault Exception (With Error Code!)
ISR_ERRNO	13; 13: General Protection Fault Exception (With Error Code!)
ISR_ERRNO	14; 14: Page Fault Exception (With Error Code!)
ISR_NOERR	15; 15: Reserved Exception
ISR_NOERR	16; 16: Floating Point Exception
ISR_NOERR	17; 17: Alignment Check Exception
ISR_NOERR	18; 18: Machine Check Exception
ISR_NOERR	19; 19: Reserved
ISR_NOERR	20; 20: Reserved
ISR_NOERR	21; 21: Reserved
ISR_NOERR	22; 22: Reserved
ISR_NOERR	23; 23: Reserved
ISR_NOERR	24; 24: Reserved
ISR_NOERR	25; 25: Reserved
ISR_NOERR	26; 26: Reserved
ISR_NOERR	27; 27: Reserved
ISR_NOERR	28; 28: Reserved
ISR_NOERR	29; 29: Reserved
ISR_NOERR	30; 30: Reserved
ISR_NOERR	31; 31: Reserved

DEF_SYSCALL	0xAC	; Acess System Call

; IRQs
; - Timer
[global Isr240]
Isr240:
	push 0
	jmp SchedulerBase
; - Assignable
%assign i	0xF1
%rep 16
	DEF_IRQ	i
%assign i i+1
%endrep

; ---------------------
; Common error handling
; ---------------------
[extern ErrorHandler]
ErrorCommon:
	pusha
	push ds
	push es
	push fs
	push gs
	
	push esp
	call ErrorHandler
	add esp, 4
	
	pop gs
	pop fs
	pop es
	pop ds
	popa
	add esp, 8	; Error Code and ID
	iret

; --------------------------
; Common System Call Handler
; --------------------------
[extern SyscallHandler]
SyscallCommon:
	pusha
	push ds
	push es
	push fs
	push gs
	
	push esp
	call SyscallHandler
	add esp, 4
	
	pop gs
	pop fs
	pop es
	pop ds
	popa
	add esp, 8	; Error Code and ID
	iret

; ------------
; IRQ Handling
; ------------
[extern IRQ_Handler]
IRQCommon:
	pusha
	push ds
	push es
	push fs
	push gs
	
	push esp
	call IRQ_Handler
	add esp, 4
	
	pop gs
	pop fs
	pop es
	pop ds
	popa
	add esp, 8	; Error Code and ID
	iret

; --------------
; Task Scheduler
; --------------
[extern Proc_Scheduler]
SchedulerBase:
	pusha
	push ds
	push es
	push fs
	push gs
	
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

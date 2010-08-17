; AcessOS Microkernel Version
;
; desctab.asm
[BITS 32]

%if USE_MP
MAX_CPUS	equ	8
%else
MAX_CPUS	equ	1
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
	dd 26*4-1, 0x00408900	; 28 Double Fault TSS
	times MAX_CPUS	dd 26*4-1, 0x00408900	; 30+ TSSes
[global gGDTPtr]
gGDTPtr:
	dw	GDT_SIZE-1
	dd	gGDT
; IDT
ALIGN 8
[global gIDT]
gIDT:
	; CS = 0x08, Type = 32-bit Interrupt (0xE = 1 110)
	times	256	dd	0x00080000,0x00000E00
[global gIDTPtr]
gIDTPtr:
	dw	256 * 16 - 1	; Limit
	dd	gIDT		; Base

[section .text]

[global Desctab_Install]
Desctab_Install:
	; Set GDT
	lgdt [gGDTPtr]
	mov ax, 0x10	; PL0 Data
	mov ss, ax
	mov ds, ax
	mov es, ax
	mov gs, ax
	mov fs, ax
	jmp 0x08:.pl0code
.pl0code:

	; Set up IDT
	; Helper Macros
	; - Set an IDT entry to an ISR
%macro SETISR	1
	mov eax, Isr%1
	mov	WORD [gIDT + %1*8], ax
	shr eax, 16
	mov	WORD [gIDT + %1*8+6], ax
	; Enable
	mov	ax, WORD [gIDT + %1*8 + 4]
	or ax, 0x8000
	mov	WORD [gIDT + %1*8 + 4], ax
%endmacro
	; Enable user calling of an ISR
%macro SET_USER	1
	or WORD [gIDT + %1*8 + 4], 0x6000
%endmacro
	; Set an ISR as a trap (leaves interrupts enabled when invoked)
%macro SET_TRAP	1
	or WORD [gIDT + %1*8 + 4], 0x0100
%endmacro

	; Error handlers
	%assign	i	0
	%rep 32
	SETISR	i
	%assign i i+1
	%endrep
	
	; User Syscall
	SETISR	0xAC
	SET_USER	0xAC
	SET_TRAP	0xAC	; Interruptable
	
	; MP ISRs
	%if USE_MP
	SETISR	0xEE	; 0xEE Timer
	SETISR	0xEF	; 0xEF Spurious Interrupt
	%endif

	; IRQs
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
	;xchg bx, bx
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

%if USE_MP
[global Isr0xEE]
[extern SchedulerBase]
; AP's Timer Interrupt
Isr0xEE:
	push 0
	xchg bx, bx	; MAGIC BREAK
	jmp SchedulerBase
; Spurious Interrupt
[global Isr0xEF]
Isr0xEF:
	xchg bx, bx	; MAGIC BREAK
	iret
%endif

; IRQs
; - Timer
[global Isr240]
[global Isr240.jmp]
[extern SchedulerBase]
[extern SetAPICTimerCount]
Isr240:
	push 0
	;xchg bx, bx	; MAGIC BREAK
Isr240.jmp:
	%if USE_MP
	jmp SetAPICTimerCount	; This is reset once the bus speed has been calculated
	%else
	jmp SchedulerBase
	%endif
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
	;xchg bx, bx	; MAGIC BREAK
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
[global IRQCommon]
IRQCommon:
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

; AcessOS Microkernel Version
; Start.asm

[bits 32]

KERNEL_BASE	equ 0xC0000000
%define MAX_CPUS	16

[extern __load_addr]
[extern __bss_start]
[extern gKernelEnd]
[section .multiboot]
mboot:
	; Multiboot macros to make a few lines later more readable
	MULTIBOOT_PAGE_ALIGN	equ 1<<0
	MULTIBOOT_MEMORY_INFO	equ 1<<1
	MULTIBOOT_REQVIDMODE	equ 1<<2
	MULTIBOOT_HEADER_MAGIC	equ 0x1BADB002
	MULTIBOOT_HEADER_FLAGS	equ MULTIBOOT_PAGE_ALIGN | MULTIBOOT_MEMORY_INFO | MULTIBOOT_REQVIDMODE
	MULTIBOOT_CHECKSUM	equ -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)
	
	; This is the GRUB Multiboot header. A boot signature
	dd MULTIBOOT_HEADER_MAGIC
	dd MULTIBOOT_HEADER_FLAGS
	dd MULTIBOOT_CHECKSUM
	
	dd mboot; - KERNEL_BASE	;Location of Multiboot Header
	dd 0	; load_addr
	dd 0	; load_end_addr
	dd 0	; bss_end_addr
	dd 0	; entry_addr
	
	dd 0	; Mode type (0: LFB)
	dd 0	; Width (no preference)
	dd 0	; Height (no preference)
	dd 32	; Depth (32-bit preferred)
	
; Multiboot 2 Header
;mboot2:
;	MULTIBOOT2_HEADER_MAGIC	equ 0xE85250D6
;	MULTIBOOT2_HEADER_ARCH	equ 0
;	MULTIBOOT2_HEADER_LENGTH	equ (mboot2_end-mboot2)
;	MULTIBOOT2_CHECKSUM	equ -(MULTIBOOT2_HEADER_MAGIC + MULTIBOOT2_HEADER_ARCH + MULTIBOOT2_HEADER_LENGTH)
;	
;	dd MULTIBOOT2_HEADER_MAGIC
;	dd MULTIBOOT2_HEADER_ARCH
;	dd MULTIBOOT2_HEADER_LENGTH
;	dd MULTIBOOT2_CHECKSUM
;	; MBoot2 Address Header
;	dw	2, 0
;	dd	8 + 16
;	dd	mboot2	; Location of Multiboot Header
;	dd	__load_addr - KERNEL_BASE	; Kernel Load base
;	dd	__bss_start - KERNEL_BASE	; Kernel Data End
;	dd	gKernelEnd - KERNEL_BASE	; Kernel BSS End
;	; MBoot2 Entry Point Tag
;	dw	3, 0
;	dd	8 + 4
;	dd	start - KERNEL_BASE
;	; MBoot2 Module Alignment Tag
;	dw	6, 0
;	dd	12	; ???
;	dd	0	; Search me, seems it wants padding
;	; Terminator
;	dw	0, 0
;	dd	8
;mboot2_end:
	
[section .inittext]
[extern kmain]
[extern Desctab_Install]
[global start]
start:
	; Just show we're here
	mov WORD [0xB8000], 0x0741	; 'A'
	
	; Set up stack
	mov esp, Kernel_Stack_Top
	
	; Start Paging
	mov ecx, gaInitPageDir - KERNEL_BASE
	mov cr3, ecx
	mov ecx, cr0
	or ecx, 0x80010000	; PG and WP
	mov cr0, ecx
	
	mov WORD [0xB8002], 0x0763	; 'c'
	
	; Set GDT
	lgdt [gGDTPtr]
	mov cx, 0x10	; PL0 Data
	mov ss, cx
	mov ds, cx
	mov es, cx
	mov gs, cx
	mov fs, cx
	mov WORD [0xB8004], 0x0765	; 'e'
	jmp 0x08:.higher_half
.higher_half:
	
	mov WORD [0xB8006], 0x0773	; 's'
	
	push ebx	; Multiboot Info
	push eax	; Multiboot Magic Value
	; NOTE: These are actually for kmain
	
	call Desctab_Install
	mov WORD [0xB8008], 0x0773	; 's'

	; Call the kernel
	mov WORD [0xB800A], 0x0732	; '2'
	call kmain

	; Halt the Machine
	cli
.hlt:
	hlt
	jmp .hlt

; 
; Multiprocessing AP Startup Code (Must be within 0 - 0x10FFF0)
;
%if USE_MP
[extern gIDTPtr]
[extern gpMP_LocalAPIC]
[extern giMP_TimerCount]
[extern gaAPIC_to_CPU]
[extern gaCPUs]
[extern giNumInitingCPUs]
[extern MM_NewKStack]
[extern Proc_InitialiseSSE]

lGDTPtr:	; Local GDT Pointer
	dw	3*8-1
	dd	gGDT-KERNEL_BASE

[bits 16]
[global APWait]
APWait:
	;xchg bx, bx
.hlt:
	;hlt
	jmp .hlt
[extern Proc_Reschedule]
[global APStartup]
APStartup:
	;xchg bx, bx	; MAGIC BREAK!
	; Load initial GDT
	mov ax, 0xFFFF
	mov ds, ax
	lgdt [DWORD ds:lGDTPtr-0xFFFF0]
	; Enable PMode in CR0
	mov eax, cr0
	or al, 1
	mov cr0, eax
	; Jump into PMode
	jmp 08h:DWORD .ProtectedMode
[bits 32]
.ProtectedMode:
	; Load segment registers
	mov ax, 0x10
	mov ss, ax
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	; Start Paging
	mov eax, gaInitPageDir - KERNEL_BASE
	mov cr3, eax
	mov eax, cr0
	or eax, 0x80010000	; PG and WP
	mov cr0, eax
	; Jump to higher half
	lea eax, [.higherHalf]
	jmp eax
.higherHalf:
	; Load True GDT & IDT
	lgdt [gGDTPtr]
	lidt [gIDTPtr]

	mov ebp, [gpMP_LocalAPIC]
	mov ebx, [ebp+0x20]	; Read ID
	shr ebx, 24
	;xchg bx, bx	; MAGIC BREAK
	; BL is now local APIC ID
	mov cl, BYTE [gaAPIC_to_CPU+ebx]
	xor ebx, ebx
	mov bl, cl
	; BL is now the CPU ID
	mov dr1, ebx	; Save the CPU number to a debug register
	; Mark the CPU as up
	mov BYTE [gaCPUs+ebx*8+1], 1
	; Decrement the remaining CPU count
	dec DWORD [giNumInitingCPUs]
	
	; Create a stack
	lea edx, [ebx+1]
	shl edx, 5+2	; *32 *4
	lea esp, [gInitAPStacks+edx]
	call MM_NewKStack
	mov esp, eax
	
	; Set TSS
	lea ecx, [ebx*8+0x30]
	ltr cx
	
	;xchg bx, bx	; MAGIC_BREAK
	; Enable Local APIC
	mov DWORD [ebp+0x0F0], 0x000001EF	; Spurious Interrupt Vector (0xEF)
	mov ecx, DWORD [giMP_TimerCount]
	mov DWORD [ebp+0x380], ecx	; Set Initial Count
	mov DWORD [ebp+0x320], 0x000200EE	; Enable timer on IVT#0xEE
	mov DWORD [ebp+0x330], 0x000100E0	; ##Enable thermal sensor on IVT#0xE0
	mov DWORD [ebp+0x340], 0x000100D0	; ##Enable performance counters on IVT#0xD0
	mov DWORD [ebp+0x350], 0x000100D1	; ##Enable LINT0 on IVT#0xD1
	mov DWORD [ebp+0x360], 0x000100D2	; ##Enable LINT1 on IVT#0xD2
	mov DWORD [ebp+0x370], 0x000100E1	; ##Enable Error on IVT#0xE1
	mov DWORD [ebp+0x0B0], 0	; Send an EOI (just in case)

	; Initialise SSE support
	call Proc_InitialiseSSE
	
	; CPU is now marked as initialised

.hlt:
	sti
	call Proc_Reschedule	
	hlt
	jmp .hlt
%endif

;
;
;
[section .text]
[global GetEIP]
GetEIP:
	mov eax, [esp]
	ret

; int CallWithArgArray(void *Ptr, int NArgs, Uint *Args)
; Call a function passing the array as arguments
[global CallWithArgArray]
CallWithArgArray:
	push ebp
	mov ebp, esp
	mov ecx, [ebp+12]	; Get NArgs
	mov edx, [ebp+16]

.top:
	mov eax, [edx+ecx*4-4]
	push eax
	loop .top
	
	mov eax, [ebp+8]
	call eax
	lea esp, [ebp]
	pop ebp
	ret

[section .data]
; GDT
GDT_SIZE	equ	(1+2*2+1+MAX_CPUS)*8
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

[section .initpd]
[global gaInitPageDir]
[global gaInitPageTable]
align 4096
gaInitPageDir:
	dd	gaInitPageTable-KERNEL_BASE+3	; 0x000 - Low kernel
	times 0x300-0x000-1	dd	0
	dd	gaInitPageTable-KERNEL_BASE+3	; 0xC00 - High kernel
	times 0x3F0-0x300-1	dd	0
	dd	gaInitPageDir-KERNEL_BASE+3 	; 0xFC0 - Fractal
	times 0x400-0x3F0-1	dd	0
align 4096
gaInitPageTable:
	%assign i 0
	%rep 1024
	dd	i*0x1000+3
	%assign i i+1
	%endrep
[global Kernel_Stack_Top]
ALIGN 4096
	times 1024	dd	0
Kernel_Stack_Top:
gInitAPStacks:
	times 32*MAX_CPUS	dd	0

; vim: ft=nasm ts=8	

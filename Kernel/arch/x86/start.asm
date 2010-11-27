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
	MULTIBOOT_HEADER_MAGIC	equ 0x1BADB002
	MULTIBOOT_HEADER_FLAGS	equ MULTIBOOT_PAGE_ALIGN | MULTIBOOT_MEMORY_INFO
	MULTIBOOT_CHECKSUM	equ -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)
	
	; This is the GRUB Multiboot header. A boot signature
	dd MULTIBOOT_HEADER_MAGIC
	dd MULTIBOOT_HEADER_FLAGS
	dd MULTIBOOT_CHECKSUM
	dd mboot; - KERNEL_BASE	;Location of Multiboot Header
	
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
	
[section .text]
[extern kmain]
[global start]
start:
	
	; Set up stack
	mov esp, Kernel_Stack_Top
	
	; Start Paging
	mov ecx, gaInitPageDir - KERNEL_BASE
	mov cr3, ecx
	
	mov ecx, cr0
	or	ecx, 0x80010000	; PG and WP
	mov cr0, ecx
	
	lea ecx, [.higherHalf]
	jmp ecx
.higherHalf:

	; Call the kernel
	push ebx	; Multiboot Info
	push eax	; Multiboot Magic Value
	call kmain

	; Halt the Machine
	cli
.hlt:
	hlt
	jmp .hlt

; 
; Multiprocessing AP Startup Code (Must be within 0x10FFF0)
;
%if USE_MP
[extern gGDT]
[extern gGDTPtr]
[extern gIDTPtr]
[extern gpMP_LocalAPIC]
[extern giMP_TimerCount]
[extern gaAPIC_to_CPU]
[extern gaCPUs]
[extern giNumInitingCPUs]
[extern MM_NewKStack]

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
[global APStartup]
APStartup:
	;xchg bx, bx	; MAGIC BREAK!
	mov ax, 0xFFFF
	mov ds, ax
	lgdt [DWORD ds:lGDTPtr-KERNEL_BASE-0xFFFF0]
	mov eax, cr0
	or	al, 1
	mov	cr0, eax
	jmp 08h:DWORD .ProtectedMode-KERNEL_BASE
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
	or	eax, 0x80010000	; PG and WP
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
	; CL is now local APIC ID
	mov cl, BYTE [gaAPIC_to_CPU+ebx]
	xor ebx, ebx
	mov bl, cl
	; BL is now the CPU ID
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
	; Save the CPU number to a debug register
	mov dr1, ebx
	
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
	
	; CPU is now marked as initialised
	sti
	;xchg bx, bx	; MAGIC BREAK
.hlt:
	hlt
	jmp .hlt
%endif

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

[section .initpd]
[global gaInitPageDir]
[global gaInitPageTable]
align 0x1000
gaInitPageDir:
	dd	gaInitPageTable-KERNEL_BASE+3	; 0x00
	times 1024-256-1	dd	0
	dd	gaInitPageTable-KERNEL_BASE+3	; 0xC0
	times 256-1	dd	0
align 0x1000
gaInitPageTable:
	%assign i 0
	%rep 1024
	dd	i*0x1000+3
	%assign i i+1
	%endrep
[global Kernel_Stack_Top]
ALIGN 0x1000
	times 1024	dd	0
Kernel_Stack_Top:
gInitAPStacks:
	times 32*MAX_CPUS	dd	0
	

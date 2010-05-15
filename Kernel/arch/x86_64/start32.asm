
[BITS 32]

KERNEL_BASE	equ	0xFFFF800000000000

[section .multiboot]
mboot:
	MULTIBOOT_MAGIC	equ	0x1BADB002
	dd	MULTIBOOT_MAGIC

[extern start64]

[section .text]
[global start]
start:
	; Enable PAE
	mov eax, cr4
	or eax, 0x80|0x20
	mov cr4, eax

	; Load PDP4
	mov eax, gInitialPML4 - KERNEL_BASE
	mov cr3, eax

	; Enable long/compatability mode
	mov ecx, 0xC0000080
	rdmsr
	or ax, 0x100
	wrmsr

	; Enable paging
	mov eax, cr0
	or eax, 0x80000000
	mov cr0, eax

	; Load GDT
	lgdt [gGDTPtr - KERNEL_BASE]
	mov ax, 0x10
	mov ss, ax
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax

	jmp 0x08:start64 - KERNEL_BASE

[section .data]
[global gGDT]
gGDT:
	dd	0,0
	dd	0x00000000, 0x00209800	; 0x08: 64-bit Code
	dd	0x00000000, 0x00009000	; 0x10: 64-bit Data
	dd	0x00000000, 0x00209800	; 0x18: 64-bit User Code
	dd	0x00000000, 0x00209000	; 0x20: 64-bit User Data
	dd	0x00000000, 0x00209800	; 0x38: 32-bit User Code
	dd	0x00000000, 0x00209000	; 0x30: 32-bit User Data
	times MAX_CPUS	dd	0, 0, 0, 0	; 0x38+16*n: TSS 0
gGDTPtr:
	dw	$-gGDT-1
	dd	gGDT-KERNEL_BASE
	dd	0

[section .padata]
[global gInitialPML4]
gInitialPML4:	; Covers 256 TiB (Full 48-bit Virtual Address Space)
	dd	gInitialPDP - KERNEL_BASE + 3, 0	; Identity Map Low 4Mb
	times 256-1 dq	0
	dd	gInitialPDP - KERNEL_BASE + 3, 0	; Map Low 4Mb to kernel base
	times 256-1-2 dq 0
	dd	gInitialPML4 - KERNEL_BASE + 3, 0	; Fractal Mapping
	dq	0

gInitialPDP:	; Covers 512 GiB
	dd	gInitialPD - KERNEL_BASE + 3, 0
	times 511 dq	0

gInitialPD:	; Covers 1 GiB
	dd	gInitialPT1 - KERNEL_BASE + 3, 0
	dd	gInitialPT2 - KERNEL_BASE + 3, 0

gInitialPT1:	; Covers 2 MiB
	%assign i 1
	%rep 512
	dq	i*4096+3
	%assign i i+1
	%endrep
gInitialPT2:	; 2 MiB
	%assign i 512
	%rep 512
	dq	i*4096+3
	%assign i i+1
	%endrep



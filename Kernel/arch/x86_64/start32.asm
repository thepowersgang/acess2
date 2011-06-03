
[BITS 32]

;KERNEL_BASE	equ	0xFFFF800000000000
KERNEL_BASE	equ	0xFFFFFFFF80000000

[section .multiboot]
mboot:
	; Multiboot macros to make a few lines later more readable
	MULTIBOOT_PAGE_ALIGN	equ 1<<0
	MULTIBOOT_MEMORY_INFO	equ 1<<1
	MULTIBOOT_AOUT_KLUDGE	equ 1<<16
	MULTIBOOT_HEADER_MAGIC	equ 0x1BADB002
	MULTIBOOT_HEADER_FLAGS	equ MULTIBOOT_PAGE_ALIGN | MULTIBOOT_MEMORY_INFO; | MULTIBOOT_AOUT_KLUDGE
	MULTIBOOT_CHECKSUM	equ -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)
	
	; This is the GRUB Multiboot header. A boot signature
	dd MULTIBOOT_HEADER_MAGIC
	dd MULTIBOOT_HEADER_FLAGS
	dd MULTIBOOT_CHECKSUM
	[extern __load_addr]
	[extern __bss_start]
	[extern gKernelEnd]
	; a.out kludge
	dd mboot	; Location of Multiboot Header
	dd __load_addr	; Load address
	dd __bss_start - KERNEL_BASE	; End of .data
	dd gKernelEnd - KERNEL_BASE	; End of .bss (and kernel)
	dd start - KERNEL_BASE	; Entrypoint

[extern start64]

[section .text]
[global start]
start:
	mov [gMultibootMagic - KERNEL_BASE], eax
	mov [gMultibootPtr - KERNEL_BASE], ebx

	; Check for Long Mode support
	mov eax, 0x80000000
	cpuid
	cmp eax, 0x80000001	; Compare the A-register with 0x80000001.
	mov eax, 0x80000001
	cpuid
	jb .not64bitCapable
	test edx, 1<<29
	jz .not64bitCapable

	; Enable PGE (Page Global Enable)
	; + PAE (Physical Address Extension)
	; + PSE (Page Size Extensions)
	mov eax, cr4
	or eax, 0x80|0x20|0x10
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
	jmp 0x08:start64 - KERNEL_BASE

.not64bitCapable:
	mov ah, 0x0F
	mov edi, 0xB8000
	mov esi, csNot64BitCapable - KERNEL_BASE

.loop:
	lodsb
	test al, al
	jz .hlt
	stosw
	jmp .loop
	
.hlt:
	jmp .hlt

[section .data]
[global gGDT]
gGDT:
	dd	0,0
	dd	0x00000000, 0x00209A00	; 0x08: 64-bit Code
	dd	0x00000000, 0x00009200	; 0x10: 64-bit Data
	dd	0x00000000, 0x0020FA00	; 0x18: 64-bit User Code
	dd	0x00000000, 0x0000F200	; 0x20: 64-bit User Data
	dd	0x00000000, 0x0040FA00	; 0x38: 32-bit User Code
	dd	0x00000000, 0x0040F200	; 0x30: 32-bit User Data
	times MAX_CPUS	dd	0, 0x00008900, 0, 0	; 0x38+16*n: TSS 0
gGDTPtr:
	dw	$-gGDT-1
	dd	gGDT-KERNEL_BASE
	dd	0
[global gMultibootPtr]
[global gMultibootMagic]
gMultibootMagic:
	dd	0
gMultibootPtr:
	dd	0

[section .padata]
[global gInitialPML4]
gInitialPML4:	; Covers 256 TiB (Full 48-bit Virtual Address Space)
	dd	gInitialPDP - KERNEL_BASE + 3, 0	; Identity Map Low 4Mb
	times 512-4-1 dq	0
	dd	gInitialPML4 - KERNEL_BASE + 3, 0	; Fractal Mapping
	dq	0
	dq	0
	dd	gHighPDP - KERNEL_BASE + 3, 0	; Map Low 4Mb to kernel base

gInitialPDP:	; Covers 512 GiB
	dd	gInitialPD - KERNEL_BASE + 3, 0
	times 511	dq	0

gHighPDP:	; Covers 512 GiB
	times 510	dq	0
	;dq	0 + 0x143	; 1 GiB Page from zero
	dd	gInitialPD - KERNEL_BASE + 3, 0
	dq	0

gInitialPD:	; Covers 1 GiB
	dd	gInitialPT1 - KERNEL_BASE + 3, 0
	dd	gInitialPT2 - KERNEL_BASE + 3, 0
	times 510	dq	0

gInitialPT1:	; Covers 2 MiB
	%assign i 0
	%rep 512
	dq	i*4096+0x103
	%assign i i+1
	%endrep
gInitialPT2:	; 2 MiB
	%assign i 512
	%rep 512
	dq	i*4096+0x103
	%assign i i+1
	%endrep


[section .rodata]
csNot64BitCapable:
	db "Not 64-bit Capable",0

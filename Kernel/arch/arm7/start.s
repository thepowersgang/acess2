KERNEL_BASE =	0x80000000

.section .init
interrupt_vector_table:
	b _start @ Reset
	b .
	b SyscallHandler @ SWI instruction
	b . 
	b .
	b .
	b .
	b .

.globl _start
_start:
	ldr r0, =kernel_table0-KERNEL_BASE
	mcr p15, 0, r0, c2, c0, 1	@ Set TTBR1 to r0
	mcr p15, 0, r0, c2, c0, 0	@ Set TTBR0 to r0 too (for identity)

@	mov r0, #1
@	mcr p15, 0, r0, c2, c0, 2	@ Set TTCR to 1 (50/50 split)

@	mrc p15, 0, r0, c1, c0, 0
	orr r0, r0, #1
@	orr r0, r0, #1 << 23
@	mcr p15, 0, r0, c1, c0, 0

	ldr sp, =stack+0x10000	@ Set up stack
	ldr r0, =kmain
	mov pc, r0
1:	b 1b	@ Infinite loop
_ptr_kmain:
	.long kmain

.comm stack, 0x10000	@ ; 64KiB Stack

SyscallHandler:
	b .
.section .padata
.globl kernel_table0

kernel_table0:
	.long 0x00000002	@ Identity map the first 4 MiB
	.rept 0x801
		.long 0
	.endr
	.long 0x00000002	@ Identity map the first 4 MiB
	.long 0x00100002	@ 
	.long 0x00200002	@ 
	.long 0x00300002	@ 
	.rept 0xF00 - 0x800 - 4
		.long 0
	.endr
	.long hwmap_table_0 + 0x000 - KERNEL_BASE + 1
	.long hwmap_table_0 + 0x400 - KERNEL_BASE + 1
	.long hwmap_table_0 + 0x800 - KERNEL_BASE + 1
	.long hwmap_table_0 + 0xC00 - KERNEL_BASE + 1
	.rept 0xFF8 - 0xF00 - 4
		.long 0
	.endr
	.long kernel_table1_map + 0x000 - KERNEL_BASE + 1
	.long kernel_table1_map + 0x400 - KERNEL_BASE + 1
	.long kernel_table1_map + 0x800 - KERNEL_BASE + 1
	.long kernel_table1_map + 0xC00 - KERNEL_BASE + 1
	.long kernel_table0 - KERNEL_BASE + 2	@ Sure it maps too much, but fuck that
	.rept 0x1000 - 0xFF8 - 5
		.long 0
	.endr

.globl kernel_table1_map
kernel_table1_map:
	.rept 0xF00/4
		.long 0
	.endr
	.long hwmap_table_0 - KERNEL_BASE + (1 << 4) + 3
	.rept 0xFF8/4 - 0xF00/4 - 1
		.long 0
	.endr
	.long kernel_table1_map - KERNEL_BASE + (1 << 4) + 3
	.long 0

.globl hwmap_table_0
hwmap_table_0:
	.long 0x16000000 + (1 << 4) + 3	@ Serial Port
	.rept 1024 - 1
		.long 0
	.endr
	

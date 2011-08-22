KERNEL_BASE =	0x80000000

interrupt_vector_table:
	b . @ Reset
	b .
	b SyscallHandler @ SWI instruction
	b . 
	b .
	b .
	b .
	b .

.comm stack, 0x10000	@ ; 64KiB Stack

.globl _start
_start:
	ldr sp, =stack+0x10000	@ Set up stack
	bl kmain
1:	b 1b	@ Infinite loop

SyscallHandler:
	
.section .padata
.globl kernel_table0
kernel_table0:
	.rept 0x800
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
	

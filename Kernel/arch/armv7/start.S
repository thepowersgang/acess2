
#include "include/assembly.h"

KERNEL_BASE =	0x80000000
PCI_PADDR   =	0x60000000	@ Realview
UART0_PADDR =	0x10009000	@ Realview
@
@ Exception defs taken from ARM DDI 0406B
@ 
.section .init
interrupt_vector_table:
ivt_reset:	b _start @ Reset
ivt_undef:	b .	@ #UD
ivt_svc:	b SyscallHandler @ SVC (SWI assume)
ivt_prefetch:	b DataAbort	@ Prefetch abort
ivt_data:	b DataAbort	@ Data abort
ivt_unused:	b .	@ Not Used
ivt_irq:	b IRQHandler	@ IRQ
ivt_fiq:	b .	@ FIQ (Fast interrupt)

.globl _start
_start:
	ldr r0, =kernel_table0-KERNEL_BASE
	mcr p15, 0, r0, c2, c0, 1	@ Set TTBR1 to r0
	mcr p15, 0, r0, c2, c0, 0	@ Set TTBR0 to r0 too (for identity)

	mov r0, #1
	mcr p15, 0, r0, c2, c0, 2	@ Set TTCR to 1 (50/50 split)
	
	mov r0, #3
	mcr p15, 0, r0, c3, c0, 0	@ Set Domain 0 to Manager

	mrc p15, 0, r0, c1, c0, 0
	orr r0, r0, #1
	orr r0, r0, #1 << 23
	mcr p15, 0, r0, c1, c0, 0

	@ Prepare for interrupts
	cps #19

	ldr sp, =stack+0x10000	@ Set up stack
	ldr r0, =kmain
	mov pc, r0
1:	b 1b	@ Infinite loop
_ptr_kmain:
	.long kmain

.comm stack, 0x10000	@ ; 64KiB Stack

SyscallHandler:
	b .

.globl gpIRQHandler
gpIRQHandler:	.long	0
IRQ_saved_sp:	.long	0
IRQ_saved_lr: 	.long	0
.globl IRQHandler
IRQHandler:
	sub lr, #4	@ Adjust LR to the correct value
	srsdb sp!, #19	@ Switch to supervisor mode (DDI0406B D1.6.5) (actually SRSFD)
	cpsie i, #19

	PUSH_GPRS

	ldr r0, =csIRQ_Tag
	ldr r1, =csIRQ_Fmt
	ldr r4, =Log_Debug
	blx r4
	
	@ Call the registered handler
	ldr r0, gpIRQHandler
	blx r0

	@ Restore CPU state
	POP_GPRS
	rfeia sp!	@ Pop state (actually RFEFD)
	bx lr

.globl DataAbort
DataAbort:
	sub lr, #8	@ Adjust LR to the correct value
	srsdb sp!, #19	@ Switch to supervisor mode (DDI0406B D1.6.5) (actually SRSFD)
	cpsie i, #19
	PUSH_GPRS

	mov r2, lr
	ldr r1, =csDataAbort_Fmt
	ldr r0, =csDataAbort_Tag
	ldr r4, =Log_Error
	blx r4
	b .

	POP_GPRS
	rfeia sp!	@ Pop state (actually RFEFD)
	bx lr

csIRQ_Tag:
csDataAbort_Tag:
	.asciz "ARMv7"
csIRQ_Fmt:
	.asciz "IRQ"
csDataAbort_Fmt:
	.asciz "Data Abort at %p"

.comm irqstack, 0x1000

.section .padata
.globl kernel_table0

kernel_table0:
	.long 0x00000002	@ Identity map the first 1 MiB
	.rept 0x7FC - 1
		.long 0
	.endr
	.long user_table1_map + 0x000 - KERNEL_BASE + 1
	.long user_table1_map + 0x400 - KERNEL_BASE + 1
	.long 0	@ user_table1_map + 0x800 - KERNEL_BASE + 1
	.long 0	@ user_table1_map + 0xC00 - KERNEL_BASE + 1
	@ 0x80000000 - User/Kernel split
	.long 0x00000002	@ Map first 4 MiB to 2GiB
	.long 0x00100002	@ 
	.long 0x00200002	@ 
	.long 0x00300002	@ 
	.rept 0xF00 - 0x800 - 4
		.long 0
	.endr
#if PCI_PADDR
	.long PCI_PADDR +  0*(1 << 20) + 2	@ Map PCI config space
	.long PCI_PADDR +  1*(1 << 20) + 2
	.long PCI_PADDR +  2*(1 << 20) + 2
	.long PCI_PADDR +  3*(1 << 20) + 2
	.long PCI_PADDR +  4*(1 << 20) + 2
	.long PCI_PADDR +  5*(1 << 20) + 2
	.long PCI_PADDR +  6*(1 << 20) + 2
	.long PCI_PADDR +  7*(1 << 20) + 2
	.long PCI_PADDR +  8*(1 << 20) + 2
	.long PCI_PADDR +  9*(1 << 20) + 2
	.long PCI_PADDR + 10*(1 << 20) + 2
	.long PCI_PADDR + 11*(1 << 20) + 2
	.long PCI_PADDR + 12*(1 << 20) + 2
	.long PCI_PADDR + 13*(1 << 20) + 2
	.long PCI_PADDR + 14*(1 << 20) + 2
	.long PCI_PADDR + 15*(1 << 20) + 2
#else
	.rept 16
		.long 0
	.endr
#endif
	.long hwmap_table_0 + 0x000 - KERNEL_BASE + 1
	.long hwmap_table_0 + 0x400 - KERNEL_BASE + 1
	.long hwmap_table_0 + 0x800 - KERNEL_BASE + 1
	.long hwmap_table_0 + 0xC00 - KERNEL_BASE + 1
	.rept 0xFF8 - 0xF00 - 16 - 4
		.long 0
	.endr
	@ Page fractals
	.long kernel_table1_map + 0x000 - KERNEL_BASE + 1
	.long kernel_table1_map + 0x400 - KERNEL_BASE + 1
	.long kernel_table1_map + 0x800 - KERNEL_BASE + 1
	.long kernel_table1_map + 0xC00 - KERNEL_BASE + 1
	@ Top level fractals
	.long 0 	@ removed for alignment constraints, using the KERNEL_BASE identity mapping instead
	.rept 0x1000 - 0xFF8 - 5
		.long 0
	.endr

@ PID0 user table
.globl user_table1_map
user_table1_map:	@ Size = 4KiB
	.rept 0x7F8/4
		.long 0
	.endr
	.long kernel_table0 - KERNEL_BASE + (1 << 4) + 3
	.long user_table1_map - KERNEL_BASE + (1 << 4) + 3
	.rept 0x800/4
		.long 0
	.endr

.globl kernel_table1_map
kernel_table1_map:	@ Size = 4KiB
	.rept 0xF00/4
		.long 0
	.endr
	.long hwmap_table_0 - KERNEL_BASE + (1 << 4) + 3
	.rept 0xFF8/4 - 0xF00/4 - 1
		.long 0
	.endr
	.long kernel_table1_map - KERNEL_BASE + (1 << 4) + 3
	.long 0

@ Hardware mappings 
.globl hwmap_table_0
hwmap_table_0:
	.long UART0_PADDR + (1 << 4) + 3	@ UART0
	.rept 1024 - 1
		.long 0
	.endr
	

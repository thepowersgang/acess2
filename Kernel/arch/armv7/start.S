
#include "include/assembly.h"
#include "include/options.h"

@
@ Exception defs taken from ARM DDI 0406B
@ 
.section .init
interrupt_vector_table:
ivt_reset:	b _start	@ 0x00 Reset
ivt_undef:	b .     	@ 0x04 #UD
ivt_svc:	b SVC_Handler	@ 0x08 SVC (used to be called SWI)
ivt_prefetch:	b PrefetchAbort	@ 0x0C Prefetch abort
ivt_data:	b DataAbort	@ 0x10 Data abort
ivt_unused:	b .     	@ 0x14 Not Used
ivt_irq:	b IRQHandler	@ 0x18 IRQ
ivt_fiq:	b .     	@ 0x1C FIQ (Fast interrupt)

.globl _start
_start:
	ldr r2, =UART0_PADDR
	mov r1, #'A'
	str r1, [r2]	

	ldr r0, =kernel_table0-KERNEL_BASE
	mcr p15, 0, r0, c2, c0, 1	@ Set TTBR1 to r0
	mcr p15, 0, r0, c2, c0, 0	@ Set TTBR0 to r0 too (for identity)

	mov r1, #'c'
	str r1, [r2]	

	mov r0, #1
	mcr p15, 0, r0, c2, c0, 2	@ Set TTCR to 1 (50/50 split)

	mov r1, #'e'
	str r1, [r2]	
	
	mov r0, #3
	mcr p15, 0, r0, c3, c0, 0	@ Set Domain 0 to Manager

	mov r1, #'s'
	str r1, [r2]	

	@ Enable VMSA
	mrc p15, 0, r0, c1, c0, 0
	orr r0, r0, #1
	orr r0, r0, #1 << 23
	mcr p15, 0, r0, c1, c0, 0

	ldr r2, =0xF1000000
	mov r1, #'s'
	str r1, [r2]	

	@ Enable access faults on domains 0 & 1
	mov r0, #0x55	@ 01010101b
	mcr p15, 0, r0, c3, c0, 0

	mov r1, #'2'
	str r1, [r2]	

	@
	@ Check for security extensions
	@
	mrc p15, 0, r0, c0, c1, 1
	and r0, #0xF0
	@ - Present
	ldrne r0,=KERNEL_BASE
	mcrne p15, 0, r0, c12, c0, 0	@ Set the VBAR (brings exceptions into high memory)
	@ - Absent
	mrceq p15, 0, r0, c1, c0, 0	@ Set SCTLR.V
	orreq r0, #0x2000
	mcreq p15, 0, r0, c1, c0, 0

	mov r1, #'-'
	str r1, [r2]	

	@ Prepare for interrupts
	cps #18	@ IRQ Mode
	ldr sp, =irqstack+0x1000	@ Set up stack
	cps #23	@ Abort Mode
	ldr sp, =abortstack+0x1000
	cps #19

	mov r1, #'a'
	str r1, [r2]	
	mov r1, #'r'
	str r1, [r2]	
	mov r1, #'m'
	str r1, [r2]	

	ldr sp, =0x80000000-4	@ Set up stack (top of user range)
	ldr r0, =kmain
	mov pc, r0
1:	b 1b	@ Infinite loop
_ptr_kmain:
	.long kmain

.comm irqstack, 0x1000	@ ; 4KiB Stack
.comm abortstack, 0x1000	@ ; 4KiB Stack

.extern SyscallHandler
SVC_Handler:
@	sub lr, #4
	srsdb sp!, #19	@ Save state to stack
	cpsie ifa, #19	@ Ensure we're in supervisor with interrupts enabled (should already be there)
	push {r0-r12}

	ldr r4, [lr,#-4]
	mvn r5, #0xFF000000
	and r4, r5

	tst r4, #0x1000 
	bne .arm_specifics

	push {r4}

	mov r0, sp
	ldr r4, =SyscallHandler
	blx r4
	
@	ldr r0, =csSyscallPrintRetAddr
@	ldr r1, [sp,#9*4+5*4]
@	ldr r4, =Log
@	blx r4
	
	pop {r2}	@ errno
	pop {r0,r1}	@ Ret/RetHi
	add sp, #2*4	@ Saved r2/r3

	pop {r4-r12}
	rfeia sp!	@ Pop state (actually RFEFD)
.arm_specifics:
	and r4, #0xFF
	mov r0, r4	@ Number
	mov r1, sp	@ Arguments
	
	ldr r4, =ARMv7_int_HandleSyscalls
	blx r4

	add sp, #4*4
	pop {r4-r12}
	rfeia sp!


.globl gpIRQHandler
gpIRQHandler:	.long	0
IRQ_saved_sp:	.long	0
IRQ_saved_lr: 	.long	0
.globl IRQHandler
IRQHandler:
	sub lr, #4	@ Adjust LR to the correct value
	srsdb sp!, #19	@ Switch to supervisor mode (DDI0406B D1.6.5) (actually SRSFD)
	cps #19

	PUSH_GPRS

@	ldr r0, =csIRQ_Tag
@	ldr r1, =csIRQ_Fmt
@	ldr r4, =Log_Debug
@	blx r4
	
	@ Call the registered handler
	ldr r0, gpIRQHandler
	blx r0

	@ Restore CPU state
	POP_GPRS
	cpsie i
	rfeia sp!	@ Pop state (actually RFEFD)
	bx lr

.globl DataAbort
DataAbort:
	sub lr, #8	@ Adjust LR to the correct value
	srsdb sp!, #23	@ Switch to supervisor mode (DDI0406B D1.6.5) (actually SRSFD)
@	cpsid ifa, #19
	PUSH_GPRS

	mov r3, #0	@ not a prefetch abort
	mrc p15, 0, r2, c5, c0, 0	@ Read DFSR (Data Fault Status Register) to R2
	mrc p15, 0, r1, c6, c0, 0	@ Read DFAR (Data Fault Address Register) into R1
	mov r0, lr	@ PC
	ldr r4, =MM_PageFault
	blx r4

	POP_GPRS
	rfeia sp!	@ Pop state (actually RFEFD)

.globl PrefetchAbort
PrefetchAbort:
	sub lr, #4	@ Adjust LR to the correct value
@	srsdb sp!, #19	@ Switch to supervisor mode (DDI0406B D1.6.5) (actually SRSFD)
@	cpsid ifa, #19
@	PUSH_GPRS

	ldr r0, =csAbort_Tag
	ldr r1, =csPrefetchAbort_Fmt
	mov r2, lr
	mrc p15, 0, r3, c5, c0, 0	@ Read IFSR (Instruction Fault Address Register) into R3
	ldr r4, =Log_Error
	blx r4
	b .

.section .rodata
csIRQ_Tag:
csAbort_Tag:
	.asciz "ARMv7"
csIRQ_Fmt:
	.asciz "IRQ"
csDataAbort_Fmt:
	.asciz "Data Abort - %p accessed %p, DFSR=%x Unk:%x Unk:%x"
csPrefetchAbort_Fmt:
	.asciz "Prefetch Abort at %p, IFSR=%x"
csSyscallPrintRetAddr:
	.asciz "Syscall ret to %p"

.section .padata
.globl kernel_table0

kernel_table0:
	.long 0x00000402	@ Identity map the first 1 MiB
	.rept 0x7FC - 1
		.long 0
	.endr
	.long user_table1_map + 0x000 - KERNEL_BASE + 1	@ 0x7FC00000
	.long user_table1_map + 0x400 - KERNEL_BASE + 1	@ 0x7FD00000
	.long user_table1_map + 0x800 - KERNEL_BASE + 1	@ KStacks
	.long user_table1_map + 0xC00 - KERNEL_BASE + 1
	@ 0x80000000 - User/Kernel split
	.long 0x00000402	@ Map first 4 MiB to 2GiB (KRW only)
	.long 0x00100402 	@ 
	.long 0x00200402	@ 
	.long 0x00300402	@ 
	.rept 0xF00 - 0x800 - 4
		.long 0
	.endr
#if PCI_PADDR
	.long PCI_PADDR +  0*(1 << 20) + 0x402	@ Map PCI config space
	.long PCI_PADDR +  1*(1 << 20) + 0x402
	.long PCI_PADDR +  2*(1 << 20) + 0x402
	.long PCI_PADDR +  3*(1 << 20) + 0x402
	.long PCI_PADDR +  4*(1 << 20) + 0x402
	.long PCI_PADDR +  5*(1 << 20) + 0x402
	.long PCI_PADDR +  6*(1 << 20) + 0x402
	.long PCI_PADDR +  7*(1 << 20) + 0x402
	.long PCI_PADDR +  8*(1 << 20) + 0x402
	.long PCI_PADDR +  9*(1 << 20) + 0x402
	.long PCI_PADDR + 10*(1 << 20) + 0x402
	.long PCI_PADDR + 11*(1 << 20) + 0x402
	.long PCI_PADDR + 12*(1 << 20) + 0x402
	.long PCI_PADDR + 13*(1 << 20) + 0x402
	.long PCI_PADDR + 14*(1 << 20) + 0x402
	.long PCI_PADDR + 15*(1 << 20) + 0x402
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
	.long kernel_exception_map + 0x000 - KERNEL_BASE + 1
	.long kernel_exception_map + 0x400 - KERNEL_BASE + 1
	.long kernel_exception_map + 0x800 - KERNEL_BASE + 1
	.long kernel_exception_map + 0xC00 - KERNEL_BASE + 1

@ PID0 user table
.globl user_table1_map
@ User table1 data table (only the first half is needed)
@ - Abused to provide kernel stacks in the unused half of the table
user_table1_map:	@ Size = 4KiB (only 2KiB used)
	.rept 0x800/4-1
		.long 0
	.endr
	.long user_table1_map - KERNEL_BASE + 0x13  	@ ...1FF000 = 0x7FDFF000
	@ Kernel stack zone
	.long kernel_table0 + 0x0000 - KERNEL_BASE + 0x13	@ ...200000 = 0x7FE00000
	.long kernel_table0 + 0x1000 - KERNEL_BASE + 0x13	@ ...201000 = 0x7FE01000
	.rept (0x800/4)-(MM_KSTACK_SIZE/0x1000)-2
		.long 0
	.endr
	#if MM_KSTACK_SIZE != 0x2000
	#error Kernel stack size not changed in start.S
	#endif
	.long stack + 0x0000 - KERNEL_BASE + 0x13	@ Kernel Stack
	.long stack + 0x1000 - KERNEL_BASE + 0x13	@ 

.globl kernel_table1_map
kernel_table1_map:	@ Size = 4KiB
	.rept (0xF00+16)/4
		.long 0
	.endr
	.long hwmap_table_0 - KERNEL_BASE + 0x13
	.rept 0xFF8/4 - (0xF00+16)/4 - 1
		.long 0
	.endr
	.long kernel_table1_map - KERNEL_BASE + 0x13
	.long kernel_exception_map - KERNEL_BASE + 0x13

@ Hardware mappings 
.globl hwmap_table_0
hwmap_table_0:
	.long UART0_PADDR + 0x13	@ UART0
	.rept 1024 - 1
		.long 0
	.endr
.globl kernel_exception_map
kernel_exception_map:
	@ Padding
	.rept 1024-256
		.long 0
	.endr
	@ Align to nearly the end
	.rept 256-16
		.long	0
	.endr
	.long 0x212	@ Map first page for exceptions (Kernel RO, Execute)
	.rept 16-1-2
		.long 0
	.endr
	.long gUsertextPhysStart + 0x22	@ User .text (User RO, Kernel RW, because both is COW)
	.long 0
	
.section .padata
stack:
	.space MM_KSTACK_SIZE, 0	@ Original kernel stack

// vim: ts=8, ft=armv7


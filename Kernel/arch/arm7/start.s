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
	bl main
1:	b 1b	@ Infinite loop


SyscallHandler:
	


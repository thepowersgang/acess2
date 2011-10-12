@
@ Acess2
@ C Runtime 0
@ - crt0.arm7.asm

.globl _start
_start:
	bl main
	push {r0}

	ldr r0, =_crt0_exit_handler
	ldr r0, [r0]
	tst r0, r0
	blxne r0

	pop {r0}	

	bl _exit
	b .	@ This should never be reached

.section .bss
.globl _crt0_exit_handler
_crt0_exit_handler:
	.long	0

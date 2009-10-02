#
# Acess2 Kernel
# i386 Architecture Makefile
# arch/i386/Makefile

# Assuming build machine is 32-bit ELF
#CC = gcc
#AS = nasm
#LD = ld
#OBJDUMP = objdump

CPPFLAGS	=
CFLAGS		=
ASFLAGS		= -f elf

A_OBJ  = start.ao main.o lib.o desctab.ao errors.o irq.o
A_OBJ += mm_phys.o mm_virt.o
A_OBJ += proc.o time.o

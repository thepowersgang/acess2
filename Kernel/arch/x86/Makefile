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

ifeq ($(ARCH),i386)
	ASFLAGS += -D USE_MP=0 -D USE_PAE=0
	CPPFLAGS += -DUSE_MP=0 -DUSE_PAE=0
else
	ifeq ($(ARCH),i486)
		ASFLAGS += -D USE_MP=1 -D USE_PAE=0
		CPPFLAGS += -DUSE_MP=1 -DUSE_PAE=0
	else
		ifeq ($(ARCH),i586)
			ASFLAGS += -D USE_MP=1 -D USE_PAE=1
			CPPFLAGS += -DUSE_MP=1 -DUSE_PAE=1
		endif
	endif
endif
	

A_OBJ  = start.ao main.o lib.o desctab.ao errors.o irq.o
A_OBJ += mm_phys.o mm_virt.o
A_OBJ += proc.o time.o

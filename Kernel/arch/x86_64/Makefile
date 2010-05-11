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
CFLAGS		= $(KERNEL_CFLAGS)
ASFLAGS		= -f elf

ifeq ($(ARCH),amd64)
	ASFLAGS += -D AMD64=1
	CPPFLAGS += -DAMD64=1
else
	ifeq ($(ARCH),ia64)
		ASFLAGS += -D AMD64=0 -D IA64=1
		CPPFLAGS += -DAMD64=0 -DIA64=1
	endif
endif
	

A_OBJ  = start32.ao start64.ao
A_OBJ += main.o lib.o

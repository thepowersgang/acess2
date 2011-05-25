#
# Acess2 Kernel
# i386 Architecture Makefile
# arch/i386/Makefile

AS_SUFFIX = asm

CPPFLAGS	=
CFLAGS		=
ASFLAGS		= -f elf

USE_MP=0
USE_PAE=0

ifeq ($(ARCH),i386)
	USE_MP=0
	USE_PAE=0
else ifeq ($(ARCH),i486)
	USE_MP=1
else ifeq ($(ARCH),i586)
	USE_MP=1
	USE_PAE=1
endif

#ASFLAGS		+= -D ARCH=\"$(ARCH)\" -D ARCHDIR=\"$(ARCHDIR)\"
ASFLAGS += -D USE_MP=$(USE_MP) -D USE_PAE=$(USE_PAE)
CPPFLAGS += -DUSE_MP=$(USE_MP) -DUSE_PAE=$(USE_PAE)

A_OBJ  = start.ao main.o lib.o desctab.ao errors.o irq.o
A_OBJ += mm_phys.o mm_virt.o
A_OBJ += proc.o proc.ao time.o vm8086.o
A_OBJ += kpanic.o

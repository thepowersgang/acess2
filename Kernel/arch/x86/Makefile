#
# Acess2 Kernel
# i386 Architecture Makefile
# arch/i386/Makefile

AS_SUFFIX = asm

CPPFLAGS	=
CFLAGS		=
ASFLAGS		= -f elf

USE_MP=0

ifeq ($(ARCH),i386)
	USE_MP=0
else ifeq ($(ARCH),i386-smp)
	USE_MP=1
endif

#ASFLAGS		+= -D ARCH=\"$(ARCH)\" -D ARCHDIR=\"$(ARCHDIR)\"
ASFLAGS += -D USE_MP=$(USE_MP)
CPPFLAGS += -DUSE_MP=$(USE_MP)

A_OBJ  = start.ao main.o lib.o desctab.ao errors.o irq.o
A_OBJ += mm_phys.o mm_virt.o
A_OBJ += proc.o proc.ao time.o vm8086.o
A_OBJ += kpanic.o pci.o

#
# Acess2 Kernel
# i386 Architecture Makefile
# arch/i386/Makefile

AS_SUFFIX = asm

CPPFLAGS	= 
CFLAGS		=
ASFLAGS		= -f elf

USE_MP=0

ifeq ($(PLATFORM),default)
	USE_MP=0
else ifeq ($(PLATFORM),smp)
	USE_MP=1
endif

ASFLAGS += -D USE_MP=$(USE_MP)
CPPFLAGS += -DUSE_MP=$(USE_MP)

A_OBJ  = start.ao main.o mboot.o lib.o desctab.ao errors.o irq.o
A_OBJ += mm_phys.o mm_virt.o
A_OBJ += proc.o proc.ao time.o vm8086.o
A_OBJ += kpanic.o pci.o vpci.o
A_OBJ += acpica.o

EXTERNS += ACPICA


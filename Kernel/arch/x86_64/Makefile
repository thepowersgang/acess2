#
# Acess2 Kernel
# i386 Architecture Makefile
# arch/i386/Makefile

MAX_CPUS := 4

CPPFLAGS	:= -DMAX_CPUS=$(MAX_CPUS)
CFLAGS		:= $(KERNEL_CFLAGS) -mno-sse -mno-mmx
ASFLAGS		:= -f elf64 -D MAX_CPUS=$(MAX_CPUS)
LDFLAGS 	:= -nostdlib -nodefaultlibs

ifeq ($(ARCH),amd64)
	ASFLAGS += -D AMD64=1
	CPPFLAGS += -DAMD64=1
else
	ifeq ($(ARCH),ia64)
		ASFLAGS += -D AMD64=0 -D IA64=1
		CPPFLAGS += -DAMD64=0 -DIA64=1
	endif
endif
	

A_OBJ  = start32.ao start64.ao desctab.ao
A_OBJ += main.o lib.o proc.o mm_virt.o mm_phys.o vm8086.o
A_OBJ += kernelpanic.o errors.o

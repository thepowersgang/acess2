#
# Acess2 Kernel
# i386 Architecture Makefile
# arch/i386/Makefile

MAX_CPUS := 4

AS_SUFFIX = asm

CPPFLAGS	:= -DMAX_CPUS=$(MAX_CPUS) -D USE_MP=0
CFLAGS		:= $(KERNEL_CFLAGS) -mno-sse -mno-mmx
ASFLAGS		:= -f elf64 -D MAX_CPUS=$(MAX_CPUS) -D USE_MP=0
LDFLAGS 	:= -nostdlib -nodefaultlibs

ifeq ($(ARCH),amd64)
	ASFLAGS += -D AMD64=1
	CPPFLAGS += -DAMD64=1
else
	ifeq ($(ARCH),x86_64)
		ASFLAGS += -D AMD64=0 -D X86_64=1
		CPPFLAGS += -DAMD64=0 -DX86_64=1
	endif
endif
	

A_OBJ := start32.ao start64.ao desctab.ao
A_OBJ += main.o lib.o proc.o mm_virt.o mm_phys.o
A_OBJ += kernelpanic.o errors.o time.o pci.o
A_OBJ += vm8086.o
# rme.o

POSTBUILD = objcopy $(BIN) -F elf32-i386 $(BIN)

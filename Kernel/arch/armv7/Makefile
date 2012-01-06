#
# Acess2 Kernel
# arm7 Architecture Makefile
# arch/arm7/Makefile

CPPFLAGS	=
CFLAGS		=
ASFLAGS		=

CPPFLAGS += -DMMU_PRESENT=1
LDFLAGS += `$(CC) --print-libgcc-file-name`

A_OBJ  = start.ao main.o lib.o lib.ao time.o pci.o debug.o
A_OBJ += mm_phys.o mm_virt.o proc.o proc.ao

#main.c: Makefile.BuildNum.$(ARCH)

POSTBUILD = objcopy $(BIN) -F binary $(BIN)

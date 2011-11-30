#
# Acess2 Kernel
# arm7 Architecture Makefile
# arch/arm7/Makefile

CPPFLAGS	=
CFLAGS		=
ASFLAGS		=

PCI_ADDRESS=0
ifeq ($(ARCH),integrator-cp)
	MMU_PRESENT=1
else
	MMU_PRESENT=1
endif



ASFLAGS += -DCONFIG_is_$(CONFIG)=1
CPPFLAGS += -DMMU_PRESENT=$(MMU_PRESENT) -DPCI_ADDRESS=$(PCI_ADDRESS) -DCONFIG_is_$(CONFIG)=1
LDFLAGS += `$(CC) --print-libgcc-file-name`

A_OBJ  = start.ao main.o lib.o lib.ao time.o pci.o debug.o
A_OBJ += mm_phys.o mm_virt.o proc.o proc.ao

main.c: Makefile.BuildNum.$(ARCH)


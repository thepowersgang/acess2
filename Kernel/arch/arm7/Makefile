#
# Acess2 Kernel
# arm7 Architecture Makefile
# arch/arm7/Makefile

CPPFLAGS	=
CFLAGS		=
ASFLAGS		=

ifeq ($(ARCH),integrator-cp)
	MMU_PRESENT=1
else
	MMU_PRESENT=1
endif



#ASFLAGS += -D USE_MP=$(USE_MP) -D USE_PAE=$(USE_PAE)
CPPFLAGS += -DMMU_PRESENT=$(MMU_PRESENT)

A_OBJ  = start.ao main.o

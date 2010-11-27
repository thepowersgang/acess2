# Acess2 Basic C Library
# Makefile

-include ../Makefile.cfg

CPPFLAGS += 
CFLAGS   += 
ASFLAGS  +=
LDFLAGS  += -soname libc.so -Map map.txt -lgcc

OBJ = stub.o heap.o stdlib.o env.o fileIO.o string.o
DEPFILES := $(OBJ:%.o=%.d)
# signals.o
BIN = libc.so

include ../Makefile.tpl

#all: $(OUTPUTDIR)Libs/crt0.o

# C Runtime 0
#$(OUTPUTDIR)Libs/crt0.o: crt0.asm
#	@echo --- $(AS) -o $@
#	@$(AS) $(ASFLAGS) -o $@ $<

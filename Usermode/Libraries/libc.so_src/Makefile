# Acess2 Basic C Library
# Makefile

-include ../Makefile.cfg

CPPFLAGS += 
CFLAGS   += -Werror -Wextra
ASFLAGS  +=
LDFLAGS  += -soname libc.so -Map map.txt -lgcc

INCFILES := stdio.h stdlib.h

OBJ  = stub.o heap.o stdlib.o env.o fileIO.o string.o select.o rand.o
OBJ += perror.o scanf.o
OBJ += arch/$(ARCHDIR).ao
# signals.o
DEPFILES := $(OBJ:%.o=%.d)
BIN = libc.so

include ../Makefile.tpl


# Acess2 Basic C Library
# Makefile

-include ../Makefile.cfg

CPPFLAGS += 
CFLAGS   += -Werror -Wextra
ASFLAGS  +=
LDFLAGS  += -soname libc.so -Map map.txt

INCFILES := stdio.h stdlib.h

OBJ  = stub.o heap.o stdlib.o env.o stdio.o string.o rand.o
OBJ += perror.o scanf.o signals.o strtoi.o strtof.o
OBJ += printf.o
OBJ += arch/$(ARCHDIR).ao
# signals.o
DEPFILES := $(OBJ:%.o=%.d)
BIN = libc.so

include ../Makefile.tpl

%.no: %.c
	$(NCC) -c $< -o $@

TEST_%: TEST_%.no %.no
	$(NCC) -o $@ TEST_$*.no $*.no


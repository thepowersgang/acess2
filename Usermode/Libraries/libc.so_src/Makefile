# Acess2 Basic C Library
# Makefile

-include ../Makefile.cfg

CPPFLAGS += 
CFLAGS   += -Wall -Werror -Wextra
ASFLAGS  +=
LDFLAGS  += -Map map.txt

INCFILES := stdio.h stdlib.h

OBJ  = stub.o heap.o stdlib.o env.o stdio.o string.o rand.o
OBJ += scanf.o signals.o strtoi.o strtof.o
OBJ += printf.o time.o timeconv.o errno.o ctype.o
OBJ += arch/$(ARCHDIR).ao
# signals.o
DEPFILES := $(OBJ:%.o=%.d)
BIN = libc.so
ifeq ($(ARCHDIR),native)
 OBJ := $(filter-out heap.o,$(OBJ))
 #LDFLAGS += -l c
 BIN = libc_acess.so
endif

include ../Makefile.tpl

EXP_%.txt: TEST_%.native
	./$< > $@
	rm $<
EXP_strtoi.txt:
	echo -n "" > $@

%.native: %.c
	$(NCC) $< -o $@ -Wall -std=gnu99

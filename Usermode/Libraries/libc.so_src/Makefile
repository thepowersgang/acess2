# Acess2 Basic C Library
# Makefile

-include ../Makefile.cfg

CPPFLAGS += 
CFLAGS   += -nostdlib -Wall -Werror -Wextra
ASFLAGS  +=
LDFLAGS  += -nostdlib
PRELINK  := $(CRTI) $(CRTBEGIN) $(OUTPUTDIR)/Libs/crt0S.o
LIBS     += $(LIBGCC_PATH) $(CRTEND) $(CRTN)

INCFILES := stdio.h stdlib.h

OBJ  = stub.o heap.o stdlib.o env.o string.o rand.o
OBJ += scanf.o signals.o strtoi.o strtof.o 
OBJ += printf.o time.o timeconv.o errno.o ctype.o
OBJ += stdio.o stdio_files.o
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

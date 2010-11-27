
-include ../Makefile.cfg

BIN = ../acess.ld

.PHONY: all clean install

all: ../acess.ld

clean:
	$(RM) ../acess.ld

install: ../acess.ld

../acess.ld:	acess.ld.h
	cpp -nostdinc -U i386 -P -C $< -o $@ -D__LIBDIR=$(ACESSDIR)/Usermode/Libraries

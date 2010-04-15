#
#
#

-include ../Makefile.cfg

ASFLAGS = -felf

.PHONY: all clean install

all: ../crt0.o

install: ../crt0.o

clean:
	$(RM) ../crt0.o

../crt0.o: crt0.asm
	$(AS) $(ASFLAGS) $< -o $@

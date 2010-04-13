#
#
#

-include ../Makefile.cfg

ASFLAGS = -felf

.PHONY: all clean

all: ../crt0.o

install:

clean:
	$(RM) ../crt0.o

../crt0.o: crt0.asm
	$(AS) $(ASFLAGS) $< -o $@

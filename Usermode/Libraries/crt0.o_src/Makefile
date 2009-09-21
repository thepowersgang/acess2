#
#
#

AS = nasm
RM = rm -f

ASFLAGS = -felf

.PHONY: all clean

all: ../crt0.o

../crt0.o: crt0.asm
	$(AS) $(ASFLAGS) $< -o $@

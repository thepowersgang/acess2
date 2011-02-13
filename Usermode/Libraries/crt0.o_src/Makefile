#
#
#

-include ../Makefile.cfg

ASFLAGS = -felf
BIN = $(OUTPUTDIR)Libs/crt0.o

.PHONY: all clean install

all: $(BIN)

install: $(BIN)

clean:
	$(RM) $(BIN)

$(BIN): crt0.$(ARCHDIR).asm
	@$(MKDIR) $(dir $(BIN))
	$(AS) $(ASFLAGS) $< -o $@

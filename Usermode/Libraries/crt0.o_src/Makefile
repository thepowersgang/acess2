#
#
#

-include ../Makefile.cfg

BIN = $(OUTPUTDIR)Libs/crt0.o

.PHONY: all clean install

all: $(BIN)

install: $(BIN)

clean:
	$(RM) $(BIN)

$(BIN): crt0.$(ARCHDIR).$(ASSUFFIX)
	@mkdir -p $(dir $(BIN))
	$(AS) $(ASFLAGS) $< -o $@

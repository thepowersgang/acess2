#
# Acess2
# - Common usermode linker script
#


-include ../Makefile.cfg

BIN = $(OUTPUTDIR)Libs/acess.ld

.PHONY: all clean install

all: $(BIN)

clean:
	$(RM) $(BIN)

install: $(BIN)

$(BIN):	acess_$(ARCHDIR).ld.h
	@mkdir -p $(dir $(BIN))
	cpp -nostdinc -U i386 -P -C $< -o $@ -D__LIBDIR=$(OUTPUTDIR)Libs

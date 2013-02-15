#
# Acess2
# - Common usermode linker script
#


-include ../Makefile.cfg

BIN = $(OUTPUTDIR)Libs/acess.ld

.PHONY: all clean install utest generate_exp

all: $(BIN)

clean:
	$(RM) $(BIN)

install: $(BIN)

# How does one unit test a linker script?
utest generate_exp:
	@echo > /dev/null

$(BIN):	acess_$(ARCHDIR).ld.h
	@mkdir -p $(dir $(BIN))
	cpp -nostdinc -U i386 -P -C $< -o $@ -D__LIBDIR=$(OUTPUTDIR)Libs

acess_$(ARCHDIR).ld.h:
	$(LD) --verbose | awk '{ if( substr($$0,0,5) == "====="){ bPrint = !bPrint; } else { if(bPrint){ print $$0;} } }' | sed 's/SEARCH_DIR\(.*\)/SEARCH_DIR(__LIBDIR)/' > $@

include $(BASE)header.mk

# Variables
BIN = $(OUTPUTDIR)Libs/acess.ld

# Rules
.PHONY: all-$(DIR)

all-$(DIR): $(BIN)
clean-$(DIR):
	$(RM) $(BIN)

$(BIN):	$(DIR)/acess_$(ARCHDIR).ld.h
	@mkdir -p $(dir $(BIN))
	cpp -nostdinc -U i386 -P -C $< -o $@ -D__LIBDIR=$(OUTPUTDIR)Libs

$(DIR)/acess_$(ARCHDIR).ld.h:
	$(LD) --verbose | awk '{ if( substr($$0,0,5) == "====="){ bPrint = !bPrint; } else { if(bPrint){ print $$0;} } }' | sed 's/SEARCH_DIR\(.*\)/SEARCH_DIR(__LIBDIR)/' > $@

include $(BASE)footer.mk

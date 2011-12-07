# Acess2 - ld-acess
include $(BASE)header.mk

# Variables
SRCS := main.c lib.c loadlib.c export.c elf.c pe.c
SRCS += arch/$(ARCHDIR).$(ASSUFFIX)
BIN  := $(OUTPUTDIR)Libs/ld-acess.so
XOBJ := $(call fcn_mkobj,_stublib.o)
XBIN := $(OUTPUTDIR)Libs/libld-acess.so

CFLAGS-$(DIR) := -Wall -fno-builtin -fno-leading-underscore -fno-stack-protector -fPIC -g
CPPFLAGS-$(DIR) := $(CPPFLAGS-$(PDIR))
LDFLAGS-$(DIR) := -g -T $(DIR)/arch/$(ARCHDIR).ld -Map $(call fcn_mkobj,map.txt) --export-dynamic

OBJ := $(call fcn_src2obj,$(SRCS))
ALL_OBJ := $(ALL_OBJ) $(OBJ)
OBJ-$(DIR) := $(OBJ) $(XOBJ)
BIN-$(DIR) := $(BIN) $(XBIN)

# Rules
.PHONY: all-$(DIR) clean-$(DIR)

all-$(DIR): $(BIN-$(DIR))
clean-$(DIR): clean-%: 
	$(eval BIN=$(BIN-$*/))
	$(eval OBJ=$(OBJ-$*/))
	$(RM) $(BIN) $(OBJ)

$(BIN): $(OBJ)

# Stub library
$(XBIN): $(call fcn_mkobj,_stublib.c.o)
	@echo [LD] -shared -o libld-acess.so
	@$(LD) -shared -o $@ $<

# Handle preprocessed files
$(DIR)/%: $(DIR)/%.h
	@echo [CPP] -o $@
	@mkdir -p $(dir $@)
	@$(CPP) $(CPPFLAGS-$(DIR)) -P -D__ASSEMBLER__ $< -o $@

include $(BASE)footer.mk

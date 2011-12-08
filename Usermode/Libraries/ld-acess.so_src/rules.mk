# ld-acess
include $(BASE)header.mk

# Variables
SRCS := main.c lib.c loadlib.c export.c elf.c pe.c
SRCS += arch/$(ARCHDIR).$(ASSUFFIX)
BIN  := $(OUTPUTDIR)Libs/ld-acess.so

CFLAGS-$(DIR) := -Wall -fno-builtin -fno-leading-underscore -fno-stack-protector -fPIC -g
CPPFLAGS-$(DIR) := $(CPPFLAGS-$(PDIR))
LDFLAGS-$(DIR) := -g -T $(DIR)/arch/$(ARCHDIR).ld -Map $(call fcn_mkobj,map.txt) --export-dynamic

include $(BASE)body.mk

$(call fcn_addbin, $(OUTPUTDIR)Libs/libld-acess.so, $(call fcn_mkobj,_stublib.c.o))

# Handle preprocessed files
$(DIR)/%: $(DIR)/%.h
	@echo [CPP] -o $@
	@mkdir -p $(dir $@)
	@$(CPP) $(CPPFLAGS-$(DIR)) -P -D__ASSEMBLER__ $< -o $@

include $(BASE)footer.mk

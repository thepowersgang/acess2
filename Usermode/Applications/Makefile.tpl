#
# Acess2
# - Application Template Makefile
#

CFLAGS  += -Wall -Werror -fno-builtin -fno-stack-protector -g
LDFLAGS += 

_BIN := $(OUTPUTDIR)$(DIR)/$(BIN)
_OBJPREFIX := obj-$(ARCH)/

OBJ := $(addprefix $(_OBJPREFIX),$(OBJ))

DEPFILES := $(OBJ:%.o=%.dep)

.PHONY : all clean install

all: $(_BIN)

clean:
	@$(RM) $(OBJ) $(DEPFILES) $(_BIN) $(BIN).dsm Map.txt

install: $(_BIN)
	@$(xMKDIR) $(DISTROOT)/$(DIR); true
	$(xCP) $(_BIN) $(DISTROOT)/$(DIR)/

$(_BIN): $(OBJ)
	@mkdir -p $(dir $(_BIN))
	@echo --- $(LD) -o $@
ifneq ($(_DBGMAKEFILE),)
	$(LD) -g $(LDFLAGS) -o $@ $(OBJ) -Map Map.txt
else
	@$(LD) -g $(LDFLAGS) -o $@ $(OBJ) -Map Map.txt
endif
	@$(DISASM) $(_BIN) > $(BIN).dsm

$(OBJ): $(_OBJPREFIX)%.o: %.c
	@echo --- GCC -o $@
ifneq ($(_OBJPREFIX),)
	@mkdir -p $(_OBJPREFIX)
endif
	@$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@
	@$(CC) -M -MT $@ $(CPPFLAGS) $< -o $(_OBJPREFIX)$*.dep

-include $(DEPFILES)

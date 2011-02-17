#
# Acess2
# - Application Template Makefile
#

CFLAGS  += -Wall -Werror -fno-builtin -fno-stack-protector -g
LDFLAGS += 

DEPFILES := $(OBJ:%.o=%.d)

_BIN := $(OUTPUTDIR)$(DIR)/$(BIN)

.PHONY : all clean install

all: $(_BIN)

clean:
	@$(RM) $(OBJ) $(DEPFILES) $(_BIN) $(BIN).dsm Map.txt

install: $(_BIN)
	@$(xMKDIR) $(DISTROOT)/$(DIR)
	$(xCP) $(_BIN) $(DISTROOT)/$(DIR)/

$(_BIN): $(OBJ)
	@mkdir -p $(dir $(_BIN))
	@echo --- $(LD) -o $@
ifneq ($(_DBGMAKEFILE),)
	$(LD) -g $(LDFLAGS) -o $@ $(OBJ) -Map Map.txt
else
	@$(LD) -g $(LDFLAGS) -o $@ $(OBJ) -Map Map.txt
endif
	@objdump -d -S $(_BIN) > $(BIN).dsm

$(OBJ): %.o: %.c
	@echo --- GCC -o $@
	@$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@
	@$(CC) -M -MT $@ $(CPPFLAGS) $< -o $*.d

-include $(DEPFILES)

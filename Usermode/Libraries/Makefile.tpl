# Acess2
# - Library Common Makefile
#


_BIN := $(OUTPUTDIR)Libs/$(BIN)
_XBIN := $(addprefix $(OUTPUTDIR)Libs/,$(EXTRABIN))
_OBJPREFIX := obj-$(ARCH)/

OBJ := $(addprefix $(_OBJPREFIX),$(OBJ))

DEPFILES := $(addsuffix .dep,$(OBJ))

.PHONY: all clean install postbuild

all: $(_BIN) $(_XBIN)

clean:
	$(RM) $(_BIN) $(_XBIN) $(OBJ) $(_BIN).dsm $(DEPFILES)

install: all
	@$(xMKDIR) $(DISTROOT)/Libs; true
	$(xCP) $(_BIN) $(_XBIN) $(DISTROOT)/Libs/

$(_BIN): $(OBJ)
	@mkdir -p $(dir $(_BIN))
	@echo [LD] -o $(BIN) $(OBJ)
	@$(LD) $(LDFLAGS) -o $(_BIN) $(OBJ)
	@$(OBJDUMP) -d -S $(_BIN) > $(_BIN).dsm

$(_OBJPREFIX)%.o: %.c
	@echo [CC] -o $@
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -o $@ -c $<
	@$(CC) -M -MT $@ $(CPPFLAGS) $< -o $@.dep

$(_OBJPREFIX)%.ao: %.asm
	@echo [AS] -o $@
	@mkdir -p $(dir $@)
	@$(AS) $(ASFLAGS) -o $@ $<
	@$(AS) $(ASFLAGS) -o $@ $< -M > $@.dep

-include $(DEPFILES)

# Acess2
# - Library Common Makefile
#

DEPFILES := $(addsuffix .d,$(OBJ))

_BIN := $(OUTPUTDIR)Libs/$(BIN)
_XBIN := $(addprefix $(OUTPUTDIR)Libs/,$(EXTRABIN))

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

%.o: %.c
	@echo [CC] -o $@
	@$(CC) $(CFLAGS) -o $@ -c $<
	@$(CC) -M -MT $@ $(CPPFLAGS) $< -o $@.d

%.ao: %.asm
	@echo [AS] -o $@
	@$(AS) $(ASFLAGS) -o $@ $<
	@$(AS) $(ASFLAGS) -o $@ $< -M > $@.d

-include $(DEPFILES)

# Acess2
# - Library Common Makefile
#

DEPFILES := $(addsuffix .d,$(OBJ))

_BIN := $(OUTPUTDIR)Libs/$(BIN)

.PHONY: all clean install postbuild

all: $(_BIN) postbuild

clean:
	$(RM) $(_BIN) $(OBJ) $(_BIN).dsm $(DEPFILES)

install: all
	$(xCP) $(_BIN) $(DISTROOT)/Libs/

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

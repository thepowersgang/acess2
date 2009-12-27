#
#
#

-include ../../../Makefile.cfg

ASFLAGS = -felf
LDFLAGS = -nostdlib -shared -I/Acess/Libs/ld-acess.so -e SoMain

OBJ = core.ao vfs.ao mm.ao
BIN = ../libacess.so

.PHONY: all clean install

all: $(BIN)

clean:
	$(RM) $(BIN) $(OBJ)

install:
	$(xCP) $(BIN) $(DISTROOT)/Libs

$(BIN): $(OBJ)
	@echo --- $(LD) -shared -o $@
	@$(LD) $(LDFLAGS) -o $(BIN) $(OBJ)
	@$(STRIP) $(BIN)
	
%.ao: %.asm syscalls.inc.asm
	@echo $(AS) -o $@
	@$(AS) $(ASFLAGS) -o $@ $<

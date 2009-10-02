#
#
#

-include ../../../Makefile.cfg

ASFLAGS = -felf
LDFLAGS = -nostdlib -shared -I/Acess/Libs/ld-acess.so -e SoMain

OBJ = core.ao vfs.ao mm.ao
BIN = ../libacess.so

.PHONY: all clean

all: $(BIN)

clean:
	$(RM) $(BIN) $(OBJ)

$(BIN): $(OBJ)
	@echo --- $(LD) -shared -o $@
	@$(LD) $(LDFLAGS) -o $(BIN) $(OBJ)
	$(STRIP) $(BIN)
	cp $(BIN) $(DISTROOT)/Libs
	
%.ao: %.asm syscalls.inc.asm
	@echo $(AS) -o $@
	@$(AS) $(ASFLAGS) -o $@ $<

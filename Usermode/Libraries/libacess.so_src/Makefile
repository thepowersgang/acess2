#
#
#

AS = nasm
LD = ld
STRIP = strip
RM = rm -f

ASFLAGS = -felf
LDFLAGS = -nostdlib -shared -I/Acess/Libs/ld-acess.so -e SoMain

OBJ = core.ao vfs.ao mm.ao
BIN = ../libacess.so

.PHONY: all clean

all: $(BIN)

clean:
	$(RM) $(BIN) $(OBJ)

$(BIN): $(OBJ)
	$(LD) $(LDFLAGS) -o $(BIN) $(OBJ)
	$(STRIP) $(BIN)
	cp $(BIN) /mnt/AcessHDD/Acess2/Libs
	
%.ao: %.asm syscalls.inc.asm
	$(AS) $(ASFLAGS) -o $@ $<

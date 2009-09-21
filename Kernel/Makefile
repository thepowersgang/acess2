# Acess2 Kernel
# Root Makefile
# NOTE: Does some voodoo to allow differing architecture builds to co-exist
# - The built objects and dependency files are suffixed with the arch name
# - The final binary is Acess2.<ARCH>.bin

-include Makefile.cfg

-include arch/$(ARCHDIR)/Makefile

RM = rm -f

MAKEDEP		= $(CC) -M

CPPFLAGS	+= -I./include -I./arch/$(ARCHDIR)/include -DARCH=$(ARCH)
CFLAGS		+= -Wall -Werror -O3 -fno-stack-protector -fno-builtin -fleading-underscore
ASFLAGS		+= -D ARCH=\"$(ARCH)\"
LDFLAGS		+= -T arch/$(ARCHDIR)/link.ld

OBJ = $(addprefix arch/$(ARCHDIR)/,$(A_OBJ))
OBJ += heap.o messages.o debug.o modules.o lib.o syscalls.o system.o
OBJ += binary.o bin/elf.o
OBJ += vfs/main.o vfs/open.o vfs/acls.o vfs/dir.o vfs/io.o vfs/mount.o vfs/memfile.o vfs/nodecache.o
OBJ += vfs/fs/root.o vfs/fs/devfs.o vfs/fs/fat.o
OBJ += drv/pci.o drv/ata_x86.o drv/vterm.o drv/vga.o drv/kb.o
OBJ := $(addsuffix .$(ARCH), $(OBJ))
BIN = ../Acess2.$(ARCH).bin

DEPFILES  = $(filter %.o.$(ARCH),$(OBJ))
DEPFILES := $(DEPFILES:%.o.$(ARCH)=%.d.$(ARCH))

SRCFILES  = $(OBJ:%.o.$(ARCH)=%.c)
SRCFILES := $(SRCFILES:%.ao.$(ARCH)=%.asm)

.PHONY: all clean

all: $(BIN)

clean:
	@$(RM) $(BIN) $(OBJ) $(DEPFILES)

$(BIN): $(OBJ) arch/$(ARCHDIR)/link.ld Makefile
	@echo --- LD -o $(BIN)
	@$(LD) $(LDFLAGS) -o $(BIN) $(OBJ) -Map ../Map.$(ARCH).txt
	@objdump $(BIN) -D > $(BIN).dsm
	@cp $(BIN) /mnt/AcessFDD/
	@wc -l $(SRCFILES) > LineCounts.$(ARCH).txt
	@git commit

%.ao.$(ARCH): %.asm Makefile
	@echo --- NASM -o $@
	@$(AS) $(ASFLAGS) $< -o $@

%.o.$(ARCH): %.c Makefile
	@echo --- GCC -o $@
	@$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ -c $<
	@$(MAKEDEP) $(CPPFLAGS) -MT $@ -o $*.d.$(ARCH) $<

include/syscalls.h:	syscalls.lst Makefile
	php GenSyscalls.php

# Dependency Files
-include $(DEPFILES)

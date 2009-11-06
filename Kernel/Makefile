# Acess2 Kernel
# Root Makefile
# NOTE: Does some voodoo to allow differing architecture builds to co-exist
# - The built objects and dependency files are suffixed with the arch name
# - The final binary is Acess2.<ARCH>.bin

-include ../Makefile.cfg

-include arch/$(ARCHDIR)/Makefile

MAKEDEP		= $(CC) -M

CPPFLAGS	+= -I./include -I./arch/$(ARCHDIR)/include -DARCH=$(ARCH)
CFLAGS		+= -Wall -Werror -O3 -fno-stack-protector -fno-builtin
ASFLAGS		+= -D ARCH=\"$(ARCH)\"
LDFLAGS		+= -T arch/$(ARCHDIR)/link.ld

OBJ = $(addprefix arch/$(ARCHDIR)/,$(A_OBJ))
OBJ += heap.o messages.o debug.o modules.o lib.o syscalls.o system.o threads.o drvutil.o
OBJ += binary.o bin/elf.o
OBJ += vfs/main.o vfs/open.o vfs/acls.o vfs/dir.o vfs/io.o vfs/mount.o vfs/memfile.o vfs/nodecache.o
OBJ += vfs/fs/root.o vfs/fs/devfs.o
OBJ += $(addprefix vfs/fs/, $(addsuffix .o,$(FILESYSTEMS)))
OBJ += drv/fifo.o drv/dma.o drv/iocache.o drv/pci.o drv/kb.o drv/vga.o drv/vterm.o
OBJ += $(addprefix drv/, $(addsuffix .o,$(DRIVERS)))
OBJ := $(addsuffix .$(ARCH), $(OBJ))
MODS += $(addprefix ../Modules/, $(addsuffix .xo.$(ARCH),$(MODULES)))
BIN = ../Acess2.$(ARCH).bin

DEPFILES  = $(filter %.o.$(ARCH),$(OBJ))
DEPFILES := $(DEPFILES:%.o.$(ARCH)=%.d.$(ARCH))

SRCFILES  = $(OBJ:%.o.$(ARCH)=%.c)
SRCFILES := $(SRCFILES:%.ao.$(ARCH)=%.asm)

.PHONY: all clean apidoc

all: $(BIN)

clean:
	@$(RM) $(BIN) $(OBJ) $(DEPFILES)

apidoc:
	doxygen Doxyfile.api

$(BIN): $(OBJ) $(MODS) arch/$(ARCHDIR)/link.ld Makefile
	@echo --- LD -o $(BIN)
	@$(LD) $(LDFLAGS) -o $(BIN) $(OBJ) $(MODS) -Map ../Map.$(ARCH).txt
	@objdump $(BIN) -D > $(BIN).dsm
	cp $(BIN) $(DISTROOT)
	@wc -l $(SRCFILES) > LineCounts.$(ARCH).txt
#	@git commit -a

%.ao.$(ARCH): %.asm Makefile
	@echo --- NASM -o $@
	@$(AS) $(ASFLAGS) $< -o $@

%.o.$(ARCH): %.c Makefile
#	if exists %*/Makefile
#	@make -C %*/ all
#	else
	@echo --- GCC -o $@
	@$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ -c $<
	@$(MAKEDEP) $(CPPFLAGS) -MT $@ -o $*.d.$(ARCH) $<
#	endif

%.xo.$(ARCH):
	@make -C $* all

include/syscalls.h:	syscalls.lst Makefile
	php GenSyscalls.php

Makefile:	../Makefile.cfg arch/$(ARCHDIR)/Makefile

# Dependency Files
-include $(DEPFILES)

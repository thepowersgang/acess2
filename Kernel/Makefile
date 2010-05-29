# Acess2 Kernel
# Root Makefile
# NOTE: Does some voodoo to allow differing architecture builds to co-exist
# - The built objects and dependency files are suffixed with the arch name
# - The final binary is Acess2.<ARCH>.bin

-include ../Makefile.cfg

-include arch/$(ARCHDIR)/Makefile

-include Makefile.BuildNum.$(ARCH)

KERNEL_VERSION = 0.5

MAKEDEP		= $(CC) -M

CPPFLAGS	+= -I./include -I./arch/$(ARCHDIR)/include
CPPFLAGS	+= -DARCH=$(ARCH) -DARCHDIR=$(ARCHDIR) -DKERNEL_VERSION=$(KERNEL_VERSION) -DBUILD_NUM=$(BUILD_NUM)
CFLAGS		+= -Wall -Werror -O3 -fno-stack-protector -fno-builtin
ASFLAGS		+= -D ARCH=\"$(ARCH)\" -D ARCHDIR=\"$(ARCHDIR)\"
LDFLAGS		+= -T arch/$(ARCHDIR)/link.ld

ifeq ($(DEBUG_BUILD),yes)
	LDFLAGS += -g
	CFLAGS += -g
endif

OBJ := $(addprefix arch/$(ARCHDIR)/,$(A_OBJ))
OBJ += heap.o drvutil.o logging.o debug.o lib.o adt.o time.o
OBJ += messages.o modules.o syscalls.o system.o threads.o
OBJ += $(addprefix vfs/fs/, $(addsuffix .o,$(FILESYSTEMS)))
OBJ += drv/vterm.o drv/proc.o drv/fifo.o drv/iocache.o drv/dma.o drv/pci.o drv/kb.o drv/vga.o
OBJ += binary.o bin/elf.o bin/pe.o
OBJ += vfs/main.o vfs/open.o vfs/acls.o vfs/dir.o vfs/io.o vfs/mount.o vfs/memfile.o vfs/nodecache.o
OBJ += vfs/fs/root.o vfs/fs/devfs.o
OBJ += $(addprefix drv/, $(addsuffix .o,$(DRIVERS)))
OBJ := $(addsuffix .$(ARCH), $(OBJ))
MODS += $(addprefix ../Modules/, $(addsuffix .xo.$(ARCH),$(MODULES)))
BIN = ../Acess2.$(ARCH).bin
GZBIN = ../Acess2.$(ARCH).gz

DEPFILES  = $(filter %.o.$(ARCH),$(OBJ))
DEPFILES := $(DEPFILES:%.o.$(ARCH)=%.d.$(ARCH))

SRCFILES  = $(OBJ:%.o.$(ARCH)=%.c)
SRCFILES := $(SRCFILES:%.ao.$(ARCH)=%.asm)

.PHONY: all clean install apidoc

all: $(BIN)

clean:
	@$(RM) $(BIN) ../Acess2.$(ARCH).gz $(BIN).dsm ../Map.$(ARCH).txt LineCounts.$(ARCH).txt $(OBJ) $(DEPFILES)

install: $(BIN)
	gzip -c $(BIN) > ../Acess2.$(ARCH).gz
	$(xCP) ../Acess2.$(ARCH).gz $(DISTROOT)

apidoc:
	doxygen Doxyfile.api

$(BIN): $(OBJ) $(MODS) arch/$(ARCHDIR)/link.ld Makefile
	@echo --- LD -o $(BIN)
	@$(LD) $(LDFLAGS) -o $(BIN) $(OBJ) $(MODS) -Map ../Map.$(ARCH).txt
	$(DISASM) $(BIN) > $(BIN).dsm
	@wc -l $(SRCFILES) include/*.h > LineCounts.$(ARCH).txt
	@echo BUILD_NUM = $$(( $(BUILD_NUM) + 1 )) > Makefile.BuildNum.$(ARCH)

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

drv/proc.o.%: Makefile.BuildNum

# Dependency Files
-include $(DEPFILES)

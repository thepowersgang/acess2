# Acess2 Kernel
# Root Makefile
# NOTE: Does some voodoo to allow differing architecture builds to co-exist
# - The built objects and dependency files are suffixed with the arch name
# - The final binary is Acess2.<ARCH>.bin

-include ../Makefile.cfg

-include arch/$(ARCHDIR)/Makefile

-include Makefile.BuildNum.$(ARCH)

ifeq ($(BUILDNUM),)
BUILDNUM = 0
endif

KERNEL_VERSION = 0.5

MAKEDEP		= $(CC) -M

CPPFLAGS	+= -I./include -I./arch/$(ARCHDIR)/include -D_MODULE_NAME_=\"Kernel\"
CPPFLAGS	+= -DARCH=$(ARCH) -DARCHDIR=$(ARCHDIR) -DARCHDIR_IS_$(ARCHDIR)=1
CPPFLAGS	+= -DKERNEL_VERSION=$(KERNEL_VERSION)
CFLAGS  	+= -Wall -Werror -fno-stack-protector -fno-builtin -Wstrict-prototypes -g
CFLAGS  	+= -Wshadow -Wpointer-arith -Wcast-align -Wwrite-strings -Wmissing-prototypes -Wmissing-declarations -Wredundant-decls -Wnested-externs -Winline -Wuninitialized
LDFLAGS		+= -T arch/$(ARCHDIR)/link.ld -g

OBJDIR := obj-$(ARCH)/
#OBJSUFFIX := .$(ARCH)

ifeq ($(DEBUG_BUILD),yes)
	LDFLAGS += -g
	CFLAGS += -g
endif

ifeq ($(AS_SUFFIX),)
	AS_SUFFIX = S
endif

BUILDINFO_OBJ := $(OBJDIR)buildinfo.o$(OBJSUFFIX)
BUILDINFO_SRC := $(OBJDIR)buildinfo.c$(OBJSUFFIX)

OBJ := $(addprefix arch/$(ARCHDIR)/,$(A_OBJ))
OBJ += heap.o drvutil.o logging.o debug.o lib.o adt.o time.o
OBJ += messages.o modules.o syscalls.o system.o threads.o
OBJ += $(addprefix vfs/fs/, $(addsuffix .o,$(FILESYSTEMS)))
OBJ += drv/vterm.o drv/proc.o drv/fifo.o drv/iocache.o drv/pci.o
#OBJ += drv/kb.o drv/dma.o drv/vga.o
OBJ += binary.o bin/elf.o bin/pe.o
OBJ += vfs/main.o vfs/open.o vfs/acls.o vfs/dir.o vfs/io.o vfs/mount.o
OBJ += vfs/memfile.o vfs/nodecache.o vfs/handle.o vfs/select.o vfs/mmap.o
OBJ += vfs/fs/root.o vfs/fs/devfs.o
OBJ += $(addprefix drv/, $(addsuffix .o,$(DRIVERS)))

OBJ := $(addsuffix $(OBJSUFFIX), $(OBJ))
OBJ := $(addprefix $(OBJDIR), $(OBJ))

MODS += $(addprefix ../Modules/, $(addsuffix .xo.$(ARCH),$(MODULES)))
BIN = ../Acess2.$(ARCH).bin
GZBIN = ../Acess2.$(ARCH).gz

DEPFILES  = $(filter %.o$(OBJSUFFIX),$(OBJ))
DEPFILES := $(DEPFILES:%.o$(OBJSUFFIX)=%.dep$(OBJSUFFIX))

SRCFILES  = $(OBJ:$(OBJDIR)%.o$(OBJSUFFIX)=%.c)
SRCFILES := $(SRCFILES:$(OBJDIR)%.ao$(OBJSUFFIX)=%.$(AS_SUFFIX))

OBJ += $(BUILDINFO_OBJ)

.PHONY: all clean install apidoc

all: $(BIN)

clean:
#	$(RM) $(BIN) ../Acess2.$(ARCH).gz $(BIN).dsm ../Map.$(ARCH).txt LineCounts.$(ARCH).txt $(OBJ) $(DEPFILES)
	@$(RM) $(BIN) ../Acess2.$(ARCH).gz $(BIN).dsm ../Map.$(ARCH).txt LineCounts.$(ARCH).txt -r $(OBJDIR) $(OBJ) $(DEPFILES) $(BUILDINFO_SRC)

install: $(BIN) 
	cp $(BIN) $(BIN)_
	$(STRIP) $(BIN)_
	gzip -c $(BIN)_ > ../Acess2.$(ARCH).gz
	$(RM) $(BIN)_
	$(xCP) ../Acess2.$(ARCH).gz $(DISTROOT)

apidoc:
	doxygen Doxyfile.api

$(BIN): $(OBJ) $(MODS) arch/$(ARCHDIR)/link.ld Makefile
	@echo --- LD -o $(BIN)
	@$(LD) $(LDFLAGS) -o $(BIN) $(OBJ) $(MODS) --defsym __buildnum=$$(( $(BUILD_NUM) + 1 )) -Map ../Map.$(ARCH).txt
	$(DISASM) -S $(BIN) > $(BIN).dsm
	@wc -l $(SRCFILES) include/*.h > LineCounts.$(ARCH).txt
	@echo BUILD_NUM = $$(( $(BUILD_NUM) + 1 )) > Makefile.BuildNum.$(ARCH)
	$(POSTBUILD)

$(OBJDIR)%.ao$(OBJSUFFIX): %.$(AS_SUFFIX) Makefile
	@echo --- AS -o $@
	@mkdir -p $(dir $@)
	@$(AS) $(ASFLAGS) $< -o $@

$(OBJDIR)%.o$(OBJSUFFIX): %.c Makefile
#	if exists %*/Makefile
#	@make -C %*/ all
#	else
	@echo --- CC -o $@
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ -c $<
	@$(MAKEDEP) $(CPPFLAGS) -MT $@ -o $(OBJDIR)$*.dep$(OBJSUFFIX) $<
#	endif

%.xo.$(ARCH):
	@BUILDTYPE=static make -C $* all

include/syscalls.h include/syscalls.inc.asm:	syscalls.lst Makefile GenSyscalls.pl
	perl GenSyscalls.pl

Makefile:	../Makefile.cfg arch/$(ARCHDIR)/Makefile

$(BUILDINFO_SRC): $(filter-out $(BUILDINFO_OBJ), $(OBJ)) $(MODS) arch/$(ARCHDIR)/link.ld Makefile
	@echo "#include <acess.h>" > $@
	@echo "const char gsGitHash[] = \""`git log -n 1 | head -n 1 | awk '{print $$2}'`"\";" >> $@
	@echo "const int giBuildNumber = $(BUILD_NUM);" >> $@
$(BUILDINFO_OBJ): $(BUILDINFO_SRC)
	@echo --- CC -o $@
	@$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ -c $<

# Dependency Files
-include $(DEPFILES)

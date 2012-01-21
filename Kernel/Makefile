# Acess2 Kernel
# Root Makefile
# NOTE: Does some voodoo to allow differing architecture builds to co-exist
# - The built objects and dependency files are suffixed with the arch name
# - The final binary is Acess2.<ARCH>.bin

-include ../Makefile.cfg

-include arch/$(ARCHDIR)/Makefile

-include Makefile.BuildNum.$(ARCH)

ifeq ($(BUILD_NUM),)
BUILD_NUM = 0
endif

KERNEL_VERSION = $(ACESS_VERSION)
MAKEDEP		= $(CC) -M

ifeq ($(AS_SUFFIX),)
	AS_SUFFIX = S
endif

ASFLAGS         += -D ARCHDIR_IS_$(ARCHDIR)=1 -D PLATFORM_is_$(PLATFORM)=1
CPPFLAGS	+= -I./include -I./arch/$(ARCHDIR)/include -D_MODULE_NAME_=\"Kernel\"
CPPFLAGS	+= -D ARCH=$(ARCH) -D ARCHDIR=$(ARCHDIR) -D PLATFORM=\"$(PLATFORM)\" -D ARCHDIR_IS_$(ARCHDIR)=1 -D PLATFORM_is_$(PLATFORM)=1
CPPFLAGS	+= -D KERNEL_VERSION=$(KERNEL_VERSION)
CFLAGS  	+= -Wall -fno-stack-protector -Wstrict-prototypes -g
CFLAGS  	+= -Wshadow -Wpointer-arith -Wcast-align -Wwrite-strings -Wmissing-prototypes -Wmissing-declarations -Wredundant-decls -Wnested-externs -Winline -Wuninitialized
CFLAGS          += -O3
LDFLAGS		+= -T arch/$(ARCHDIR)/link.ld -g

ifeq ($(PLATFORM),default)
	OBJDIR := obj-$(ARCH)/
	#OBJSUFFIX := .$(ARCH)
	BIN := ../Acess2.$(ARCH).bin
	GZBIN := ../Acess2.$(ARCH).gz
else
	OBJDIR := obj-$(ARCH)-$(PLATFORM)/
	#OBJSUFFIX := .$(ARCH)-$(PLATFORM)
	BIN := ../Acess2.$(ARCH)-$(PLATFORM).bin
	GZBIN := ../Acess2.$(ARCH)-$(PLATFORM).gz
endif

ifeq ($(DEBUG_BUILD),yes)
	LDFLAGS += -g
	CFLAGS += -g
endif

BUILDINFO_OBJ := $(OBJDIR)buildinfo.o$(OBJSUFFIX)
BUILDINFO_SRC := $(OBJDIR)buildinfo.c$(OBJSUFFIX)

OBJ := $(addprefix arch/$(ARCHDIR)/,$(A_OBJ))
OBJ += heap.o drvutil.o logging.o debug.o lib.o adt.o time.o
OBJ += messages.o modules.o syscalls.o system.o
OBJ += threads.o mutex.o semaphore.o workqueue.o
OBJ += drv/proc.o drv/fifo.o drv/iocache.o drv/pci.o
OBJ += drv/vterm.o drv/vterm_font.o drv/vterm_vt100.o drv/vterm_output.o drv/vterm_input.o drv/vterm_termbuf.o
OBJ += binary.o bin/elf.o bin/pe.o
OBJ += vfs/main.o vfs/open.o vfs/acls.o vfs/dir.o vfs/io.o vfs/mount.o
OBJ += vfs/memfile.o vfs/nodecache.o vfs/handle.o vfs/select.o vfs/mmap.o
OBJ += vfs/fs/root.o vfs/fs/devfs.o
OBJ += $(addprefix drv/, $(addsuffix .o,$(DRIVERS)))

OBJ := $(addsuffix $(OBJSUFFIX), $(OBJ))
OBJ := $(addprefix $(OBJDIR), $(OBJ))

MODS += $(addprefix ../Modules/, $(addsuffix .xo.$(ARCH),$(MODULES)))

DEPFILES := $(OBJ:%$(OBJSUFFIX)=%.dep$(OBJSUFFIX))

SRCFILES  = $(OBJ:$(OBJDIR)%.o$(OBJSUFFIX)=%.c)
SRCFILES := $(SRCFILES:$(OBJDIR)%.ao$(OBJSUFFIX)=%.$(AS_SUFFIX))

OBJ += $(BUILDINFO_OBJ)

.PHONY: all clean install apidoc

all: $(BIN)

clean:
	@$(RM) $(BIN) ../Acess2.$(ARCH).gz $(BIN).dsm ../Map.$(ARCH).txt LineCounts.$(ARCH).txt
	@$(RM) -r $(OBJDIR) $(OBJ) $(DEPFILES) $(BUILDINFO_SRC)

install: $(BIN) 
	@cp $(BIN) $(BIN)_
	@$(STRIP) $(BIN)_
	@gzip -c $(BIN)_ > $(GZBIN)
	@$(RM) $(BIN)_
	$(xCP) $(GZBIN) $(DISTROOT)

apidoc:
	doxygen Doxyfile.api

$(BIN): $(OBJ) $(MODS) arch/$(ARCHDIR)/link.ld Makefile ../BuildConf/$(ARCH)/Makefile.cfg ../BuildConf/$(ARCH)/$(PLATFORM).mk
	@echo --- LD -o $(BIN)
	@$(LD) $(LDFLAGS) -o $(BIN) $(OBJ) $(MODS) --defsym __buildnum=$$(( $(BUILD_NUM) + 1 )) -Map ../Map.$(ARCH).txt
	@$(DISASM) -S $(BIN) > $(BIN).dsm
	@wc -l $(SRCFILES) include/*.h > LineCounts.$(ARCH).txt
	@echo BUILD_NUM = $$(( $(BUILD_NUM) + 1 )) > Makefile.BuildNum.$(ARCH)
	$(POSTBUILD)

$(OBJDIR)%.ao$(OBJSUFFIX): %.$(AS_SUFFIX) Makefile
	@echo --- AS -o $@
	@mkdir -p $(dir $@)
	@$(AS) $(ASFLAGS) $< -o $@
ifeq ($(AS_SUFFIX),S)
	@$(MAKEDEP) $(CPPFLAGS) -MT $@ -o $(OBJDIR)$*.ao.dep$(OBJSUFFIX) $<
endif

$(OBJDIR)%.o$(OBJSUFFIX): %.c Makefile
#	if exists %*/Makefile
#	@make -C %*/ all
#	else
	@echo --- CC -o $@
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ -c $<
	@$(MAKEDEP) $(CPPFLAGS) -MT $@ -o $(OBJDIR)$*.o.dep$(OBJSUFFIX) $<
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

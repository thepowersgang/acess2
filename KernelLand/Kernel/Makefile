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
CPPFLAGS	+= -D KERNEL_VERSION=$(KERNEL_VERSION) -ffreestanding
CFLAGS  	+= -Wall -fno-stack-protector -Wstrict-prototypes -std=gnu99 -g
CFLAGS  	+= -Wshadow -Wpointer-arith -Wcast-align -Wwrite-strings -Wmissing-prototypes -Wmissing-declarations -Wredundant-decls -Wnested-externs -Winline -Wuninitialized
CFLAGS          += -O3
LDFLAGS		+= -T arch/$(ARCHDIR)/link.ld -g
LIBGCC_PATH     := $(shell $(CC) -print-libgcc-file-name)

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
OBJ += pmemmap.o
OBJ += heap.o logging.o debug.o lib.o libc.o adt.o time.o
OBJ += drvutil_video.o drvutil_disk.o
OBJ += messages.o modules.o syscalls.o system.o
OBJ += threads.o mutex.o semaphore.o workqueue.o events.o rwlock.o
OBJ += drv/zero-one.o drv/proc.o drv/fifo.o drv/iocache.o drv/pci.o drv/vpci.o
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

# Lower to avoid taking default target from 'all'
-include $(addprefix ../../Externals/,$(addsuffix /Makefile.kinc, $(EXTERNS)))


# Creates a stripped and compressed copy of the kernel
# and installs it to the target
install: $(BIN) 
	$(xCP) $(GZBIN) $(DISTROOT)

# Compile API documentation
apidoc:
	doxygen Doxyfile.api

# Output binary
# - Links kernel
# - Disassembles it
# - Increments the build count
# - Does whatever architecture defined rules
$(BIN): $(OBJ) $(MODS) arch/$(ARCHDIR)/link.ld Makefile ../../BuildConf/$(ARCH)/Makefile.cfg ../../BuildConf/$(ARCH)/$(PLATFORM).mk
	@echo --- LD -o $(BIN)
	@$(LD) $(LDFLAGS) -o $(BIN) $(OBJ) $(MODS) $(EXTERN_OBJS) $(LIBGCC_PATH) --defsym __buildnum=$$(( $(BUILD_NUM) + 1 )) -Map ../Map.$(ARCH).txt
	@$(DISASM) -S $(BIN) > $(BIN).dsm
	@echo BUILD_NUM = $$(( $(BUILD_NUM) + 1 )) > Makefile.BuildNum.$(ARCH)
	$(POSTBUILD)
	@cp $(BIN) $(BIN)_
	@$(STRIP) $(BIN)_ || true
	@gzip -c $(BIN)_ > $(GZBIN)
	@$(RM) $(BIN)_

# Assembly Sources
$(OBJDIR)%.ao$(OBJSUFFIX): %.$(AS_SUFFIX) Makefile
	@echo --- AS -o $@
	@mkdir -p $(dir $@)
	@$(AS) $(ASFLAGS) $< -o $@
ifeq ($(AS_SUFFIX),S)
	@$(MAKEDEP) $(CPPFLAGS) -MT $@ -MP -o $(OBJDIR)$*.ao.dep$(OBJSUFFIX) $<
endif

# C Sources
$(OBJDIR)%.o$(OBJSUFFIX): %.c Makefile
	@echo --- CC -o $@
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ -c $<
	@$(MAKEDEP) $(CPPFLAGS) -MT $@ -MP -o $(OBJDIR)$*.o.dep$(OBJSUFFIX) $<

# Build-time linked modules
%.xo.$(ARCH):
	@BUILDTYPE=static make -C $* all

# System call lists
include/syscalls.h include/syscalls.inc.asm:	syscalls.lst Makefile GenSyscalls.pl
	perl GenSyscalls.pl

# Differences for the makefile
Makefile:	../../Makefile.cfg ../../BuildConf/$(ARCH)/Makefile.cfg ../../BuildConf/$(ARCH)/$(PLATFORM).mk arch/$(ARCHDIR)/Makefile

# Build-time information (git hash and build number)
$(BUILDINFO_SRC): $(filter-out $(BUILDINFO_OBJ), $(OBJ)) $(MODS) arch/$(ARCHDIR)/link.ld Makefile
	$(eval _GITHASH=$(shell git log -n 1 | head -n 1 | awk '{print $$2}'))
	$(eval _GITCHANGED=$(shell git status --porcelain | grep -c '^ M '))
	@echo "#include <acess.h>" > $@
	@echo "const char gsGitHash[] = \"$(_GITHASH)\";" >> $@
	@echo "const int giBuildNumber = $(BUILD_NUM);" >> $@
	@echo "const char gsBuildInfo[] = \"Acess2 v$(KERNEL_VERSION) $(ARCH)-$(PLATFORM)\\\\r\\\\n\"" >> $@
	@echo "                           \"Build $(shell hostname --fqdn):$(BUILD_NUM) Git $(_GITHASH) - $(_GITCHANGED) modified\";" >> $@
# Compile rule for buildinfo (needs a special one because it's not a general source file)
$(BUILDINFO_OBJ): $(BUILDINFO_SRC)
	@echo --- CC -o $@
	@$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ -c $<

# Dependency Files
-include $(DEPFILES)


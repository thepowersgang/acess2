

include $(dir $(lastword $(MAKEFILE_LIST)))../../Makefile.cfg

BIN := ../bin-$(ARCH)/libacpica.a

_VERS := $(patsubst acpica-unix-%.tar.gz,%,$(wildcard acpica-unix-*.tar.gz))
_VERS := $(sort $(_VERS))
_LATEST := $(lastword $(_VERS))

ifeq ($(ACPICAVER),)
 ifeq ($(_LATEST),)
  $(warning Unable to find an archive matching acpica-unix-%.tar.gz)
  $(warning Go to https://www.acpica.org/downloads/ to get it)
  $(error No ACPICA archive found)
 endif
 
 ifneq ($(_LATEST),$(_VERS))
  $(warning Multiple archvies found, picked $(_LATEST))
 endif
 ACPICAVER := $(_LATEST)
endif

ACPICAROOT := acpica-unix-$(ACPICAVER)/
ACPICA_ARCHIVE := acpica-unix-$(ACPICAVER).tar.gz
COMPDIR := $(ACPICAROOT)source/components/
KERNELDIR := ../../KernelLand/Kernel/

CPPFLAGS	+= -I $(KERNELDIR)include -I$(KERNELDIR)arch/$(ARCHDIR)/include -D_MODULE_NAME_=\"ACPICA\"
CPPFLAGS        += -I $(ACPICAROOT)source/include -D _ACESS -D __KERNEL__
CPPFLAGS	+= -D ARCH=$(ARCH) -D ARCHDIR=$(ARCHDIR) -D PLATFORM=\"$(PLATFORM)\" -D ARCHDIR_IS_$(ARCHDIR)=1 -D PLATFORM_is_$(PLATFORM)=1
CPPFLAGS	+= -D KERNEL_VERSION=$(KERNEL_VERSION) -ffreestanding
CFLAGS  	+= -Wall -fno-stack-protector -Wstrict-prototypes -std=gnu99 -g

SRCS := $(wildcard $(COMPDIR)*/*.c)
OBJS := $(SRCS:$(COMPDIR)%.c=obj-$(ARCH)/%.o)

ACENV_H := $(ACPICAROOT)source/include/platform/acenv.h
ACACESS_H := $(ACPICAROOT)source/include/platform/acacess.h

.PHONY: all clean

ifeq ($(words $(SRCS)),0)
extract: $(ACPICAROOT)
	@echo Re-run make to compile
endif

all: $(BIN) include

clean:
	$(RM) -r obj-$(ARCH)/ include

purge: clean
	$(RM) -r $(ACPICAROOT)

$(BIN): $(OBJS)
	@mkdir -p $(dir $@)
	@echo [AR] $@
	@ar rcu $@ $(OBJS)

include:
	ln -s $(ACPICAROOT)source/include

$(ACPICAROOT): $(ACPICA_ARCHIVE)
	tar -xf $(ACPICA_ARCHIVE)

$(ACACESS_H): acacess.h
	cp $< $@

$(ACENV_H): $(ACPICA_ARCHIVE) Makefile
	tar -x -O -f $(ACPICA_ARCHIVE) $(ACENV_H) | sed 's/aclinux/acacess/' | sed 's/_LINUX/_ACESS/' > $@

obj-$(ARCH)/%.o: $(COMPDIR)%.c $(ACENV_H) $(ACACESS_H)
	@mkdir -p $(dir $@)
	@echo [CC] -o $@
	@$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ -c $< 
	@$(CC) -M -MP -MT $@ $(CPPFLAGS) $< -o $@.dep

-include $(OBJS:%=%.dep)


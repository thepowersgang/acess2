

include $(dir $(lastword $(MAKEFILE_LIST)))../../Makefile.cfg

COMPONENTS = utilities tables
BIN := ../bin-$(ARCH)/libacpica.a

ACPICAVER = 20121114
ACPICAROOT := acpica-unix-$(ACPICAVER)/
COMPDIR := $(ACPICAROOT)source/components/
KERNELDIR := ../../KernelLand/Kernel/

CPPFLAGS	+= -I $(KERNELDIR)include -I$(KERNELDIR)arch/$(ARCHDIR)/include -D_MODULE_NAME_=\"ACPICA\"
CPPFLAGS        += -I $(ACPICAROOT)source/include -D _ACESS -D __KERNEL__
CPPFLAGS	+= -D ARCH=$(ARCH) -D ARCHDIR=$(ARCHDIR) -D PLATFORM=\"$(PLATFORM)\" -D ARCHDIR_IS_$(ARCHDIR)=1 -D PLATFORM_is_$(PLATFORM)=1
CPPFLAGS	+= -D KERNEL_VERSION=$(KERNEL_VERSION) -ffreestanding
CFLAGS  	+= -Wall -fno-stack-protector -Wstrict-prototypes -std=gnu99 -g

SRCS := $(foreach comp,$(COMPONENTS),$(wildcard $(COMPDIR)$(comp)/*.c))
OBJS := $(SRCS:$(COMPDIR)%.c=obj-$(ARCH)/%.o)

ACENV_H := $(ACPICAROOT)source/include/platform/acenv.h
ACACESS_H := $(ACPICAROOT)source/include/platform/acacess.h

.PHONY: all clean

all: $(BIN)

clean:
	$(RM) obj-$(ARCH)/

$(BIN): $(OBJS)
	@mkdir -p $(dir $@)
	@echo [AR] $@
	@ar rcu $@ $(OBJS)

#include_exp/acpi: $(ACPICAROOT)source/include
#	@mkdir -p $(dir $@)
#	ln -s ../$< $@

$(ACACESS_H): acacess.h
	cp $< $@

$(ACENV_H): acpica-unix-$(ACPICAVER).tar.gz Makefile
	tar -x -O -f acpica-unix-$(ACPICAVER).tar.gz $(ACENV_H) | sed 's/aclinux/acacess/' | sed 's/_LINUX/_ACESS/' > $@

obj-$(ARCH)/%.o: $(COMPDIR)%.c $(ACENV_H) $(ACACESS_H)
	@mkdir -p $(dir $@)
	@echo [CC] -o $@
	@$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ -c $< 
	@$(CC) -M -MP -MT $@ $(CPPFLAGS) $< -o $@.dep

-include $(OBJS:%=%.dep)


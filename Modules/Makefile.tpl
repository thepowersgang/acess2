
# Acess2 Module/Driver Templater Makefile
# Makefile.tpl

_CPPFLAGS := $(CPPFLAGS)

CFGFILES := 
CFGFILES += $(shell test -f ../../../Makefile.cfg && echo ../../../Makefile.cfg)
CFGFILES += $(shell test -f ../../Makefile.cfg && echo ../../Makefile.cfg)
CFGFILES += $(shell test -f ../Makefile.cfg && echo ../Makefile.cfg)
CFGFILES += $(shell test -f Makefile.cfg && echo Makefile.cfg)
-include $(CFGFILES)

CPPFLAGS := -I$(ACESSDIR)/Kernel/include -I$(ACESSDIR)/Kernel/arch/$(ARCHDIR)/include -DARCH=$(ARCH) $(_CPPFLAGS)
CFLAGS := -Wall -Werror -fno-stack-protector $(CPPFLAGS) -O3 -fno-builtin

ifeq ($(BUILDTYPE),dynamic)
	_SUFFIX := dyn_$(ARCH)
	ifneq ($(CATEGORY),)
		BIN := ../$(CATEGORY)_$(NAME).kmd.$(ARCH)
	else
		BIN := ../$(NAME).kmd.$(ARCH)
	endif
	CFLAGS += $(DYNMOD_CFLAGS) -fPIC
else
	_SUFFIX := st_$(ARCH)
	CFLAGS += $(KERNEL_CFLAGS)
	BIN := ../$(NAME).xo.$(ARCH)
endif

OBJ := $(addsuffix .$(_SUFFIX),$(OBJ))

DEPFILES := $(filter %.o.$(_SUFFIX),$(OBJ))
DEPFILES := $(DEPFILES:%.o.$(_SUFFIX)=%.d.$(ARCH))

.PHONY: all clean

all: $(BIN)

clean:
	$(RM) $(BIN) $(BIN).dsm $(KOBJ) $(OBJ) $(DEPFILES)

install: $(BIN)
ifeq ($(BUILDTYPE),dynamic)
	$(xCP) $(BIN) $(DISTROOT)/Modules/$(NAME).kmd.$(ARCH)
else
endif

ifeq ($(BUILDTYPE),dynamic)
$(BIN): %.kmd.$(ARCH): $(OBJ)
	@echo --- $(LD) -o $@
#	$(LD) -T $(ACESSDIR)/Modules/link.ld --allow-shlib-undefined -shared -nostdlib -o $@ $(OBJ)
	@$(LD) --allow-shlib-undefined -shared -nostdlib -o $@ $(OBJ)
	@$(DISASM) $(BIN) > $(BIN).dsm
else
$(BIN): %.xo.$(ARCH): $(OBJ)
	@echo --- $(LD) -o $@
	@$(LD) -r -o $@ $(OBJ)
endif

%.o.$(_SUFFIX): %.c Makefile ../Makefile.tpl $(CFGFILES)
	@echo --- $(CC) -o $@
	@$(CC) $(CFLAGS) -o $@ -c $<
	@$(CC) -M $(CPPFLAGS) -MT $@ -o $*.d.$(ARCH) $<

-include $(DEPFILES)


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

ifneq ($(CATEGORY),)
	FULLNAME := $(CATEGORY)_$(NAME)
else
	FULLNAME := $(NAME)
endif

ifneq ($(BUILDTYPE),static)
	_SUFFIX := dyn_$(ARCH)
	BIN := ../$(FULLNAME).kmd.$(ARCH)
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
	$(RM) $(BIN) $(BIN).dsm $(KOBJ) $(OBJ) $(DEPFILES) $(EXTRA)

install: $(BIN)
ifneq ($(BUILDTYPE),static)
	$(xCP) $(BIN) $(DISTROOT)/Modules/$(NAME).kmd.$(ARCH)
else
endif

ifneq ($(BUILDTYPE),static)
$(BIN): %.kmd.$(ARCH): $(OBJ)
	@echo --- $(LD) -o $@
#	@$(LD) -T $(ACESSDIR)/Modules/link.ld --allow-shlib-undefined -shared -nostdlib -o $@ $(OBJ)
	@$(LD) --allow-shlib-undefined -shared -nostdlib -o $@ $(OBJ) -defsym=DriverInfo=_DriverInfo_$(FULLNAME)
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

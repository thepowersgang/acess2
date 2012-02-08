
# Acess2 Module/Driver Templater Makefile
# Makefile.tpl

_CPPFLAGS := $(CPPFLAGS)

-include $(dir $(lastword $(MAKEFILE_LIST)))../Makefile.cfg

LIBINCLUDES := $(addprefix -I$(ACESSDIR)/KernelLand/Modules/,$(DEPS))
LIBINCLUDES := $(addsuffix /include,$(LIBINCLUDES))

CPPFLAGS := -I$(ACESSDIR)/KernelLand/Kernel/include -I$(ACESSDIR)/KernelLand/Kernel/arch/$(ARCHDIR)/include
CPPFLAGS += -DARCH=$(ARCH) -DARCH_is_$(ARCH) -DARCHDIR_is_$(ARCHDIR)
CPPFLAGS += $(_CPPFLAGS)
CPPFLAGS += $(LIBINCLUDES)
CFLAGS := -std=gnu99 -Wall -fno-stack-protector -g -O3

ifneq ($(CATEGORY),)
	FULLNAME := $(CATEGORY)_$(NAME)
else
	FULLNAME := $(NAME)
endif

CPPFLAGS += -D_MODULE_NAME_=\"$(FULLNAME)\"

ifneq ($(BUILDTYPE),static)
	_SUFFIX := dyn_$(ARCH)
	BIN := ../$(FULLNAME).kmd.$(ARCH)
	CFLAGS += $(DYNMOD_CFLAGS) -fPIC
else
	_SUFFIX := st_$(ARCH)
	CFLAGS += $(KERNEL_CFLAGS)
	BIN := ../$(NAME).xo.$(ARCH)
endif

OBJ := $(addprefix obj-$(_SUFFIX)/,$(OBJ))
#OBJ := $(addsuffix .$(_SUFFIX),$(OBJ))

DEPFILES := $(filter %.o,$(OBJ))
DEPFILES := $(DEPFILES:%.o=%.d)

.PHONY: all clean

all: $(BIN)

clean:
	$(RM) $(BIN) $(BIN).dsm $(KOBJ) $(OBJ) $(DEPFILES) $(EXTRA)
	$(RM) -r obj-$(_SUFFIX)

install: $(BIN)
ifneq ($(BUILDTYPE),static)
	@$(xMKDIR) $(DISTROOT)/Modules/$(ARCH); true
	$(xCP) $(BIN) $(DISTROOT)/Modules/$(ARCH)/$(NAME).kmd
else
endif


ifneq ($(BUILDTYPE),static)
$(BIN): %.kmd.$(ARCH): $(OBJ)
	@echo --- $(LD) -o $@
	@$(LD) --allow-shlib-undefined -shared -nostdlib -o $@ $(OBJ) -defsym=DriverInfo=_DriverInfo_$(FULLNAME) $(LDFLAGS)
	@$(DISASM) $(BIN) > $(BIN).dsm
else
$(BIN): %.xo.$(ARCH): $(OBJ)
	@echo --- $(LD) -o $@
	@$(LD) -r -o $@ $(OBJ) $(LDFLAGS)
endif

obj-$(_SUFFIX)/%.o: %.c Makefile $(CFGFILES)
	@echo --- $(CC) -o $@
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ -c $<
	@$(CC) -M $(CPPFLAGS) -MT $@ -o obj-$(_SUFFIX)/$*.d $<

-include $(DEPFILES)

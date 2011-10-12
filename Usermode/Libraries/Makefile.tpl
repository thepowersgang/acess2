# Acess2
# - Library Common Makefile
#


_BIN := $(OUTPUTDIR)Libs/$(BIN)
_XBIN := $(addprefix $(OUTPUTDIR)Libs/,$(EXTRABIN))
_OBJPREFIX := obj-$(ARCH)/

_LIBS := $(filter -l%,$(LDFLAGS))
_LIBS := $(patsubst -l%,$(OUTPUTDIR)Libs/lib%.so,$(_LIBS))

OBJ := $(addprefix $(_OBJPREFIX),$(OBJ))

DEPFILES := $(addsuffix .dep,$(OBJ))

.PHONY: all clean install postbuild

all: $(_BIN) $(_XBIN)

clean:
	$(RM) $(_BIN) $(_XBIN) $(OBJ) $(_BIN).dsm $(DEPFILES)

install: all
	@echo [xCP] $(DISTROOT)/Libs/$(BIN)
	@$(xMKDIR) $(DISTROOT)/Libs; true
	@$(STRIP) $(_BIN) -o $(_BIN)_
	@$(xCP) $(_BIN)_ $(DISTROOT)/Libs/$(BIN)
	@$(RM) $(_BIN)_
ifneq ($(_XBIN),)
	$(xCP) $(_XBIN) $(DISTROOT)/Libs/
endif

$(_BIN): $(OBJ) $(_LIBS)
	@mkdir -p $(dir $(_BIN))
	@echo [LD] -o $(BIN) $(OBJ)
	@$(LD) $(LDFLAGS) -o $(_BIN) $(OBJ)
	@$(DISASM) -S $(_BIN) > $(_OBJPREFIX)$(BIN).dsm

$(_OBJPREFIX)%.o: %.c
	@echo [CC] -o $@
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -o $@ -c $<
	@$(CC) -M -MT $@ $(CPPFLAGS) $< -o $@.dep

$(_OBJPREFIX)%.ao: %.$(ASSUFFIX)
	@echo [AS] -o $@
	@mkdir -p $(dir $@)
	@$(AS) $(ASFLAGS) -o $@ $<
	@$(AS) $(ASFLAGS) -o $@.dep $< -M

#$(OUTPUTDIR)Libs/libld-acess.so:
#	@make -C $(ACESSDIR)/Usermode/Libraries/ld-acess.so_src/
$(OUTPUTDIR)Libs/%:
	@make -C $(ACESSDIR)/Usermode/Libraries/$*_src/

-include $(DEPFILES)

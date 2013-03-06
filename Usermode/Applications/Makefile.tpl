#
# Acess2
# - Application Template Makefile
#

CFLAGS  += -g
LDFLAGS += -g

_BIN := $(OUTPUTDIR)$(DIR)/$(BIN)
_OBJPREFIX := obj-$(ARCH)/

_LIBS := $(filter -l%,$(LDFLAGS))
_LIBS := $(patsubst -l%,$(OUTPUTDIR)Libs/lib%.so,$(_LIBS))

ifeq ($(VERBOSE),)
V := @
else
V :=
endif

OBJ := $(addprefix $(_OBJPREFIX),$(OBJ))

DEPFILES := $(OBJ:%.o=%.dep)

.PHONY : all clean install

all: $(_BIN)

clean:
	@$(RM) $(OBJ) $(DEPFILES) $(_BIN) $(BIN).dsm
	@$(RM) -r $(_OBJPREFIX)

install: $(_BIN)
	@echo [xCP] $(DISTROOT)/$(DIR)/$(BIN)
	@$(xMKDIR) $(DISTROOT)/$(DIR); true
	@$(STRIP) $(_BIN) -o $(_BIN)_
	@$(xCP) $(_BIN)_ $(DISTROOT)/$(DIR)/$(BIN)
	@$(RM) $(_BIN)_

$(_BIN): $(OUTPUTDIR)Libs/acess.ld $(OUTPUTDIR)Libs/crt0.o $(_LIBS) $(OBJ)
	@mkdir -p $(dir $(_BIN))
	@echo [LD] -o $@
#ifeq ($(ARCHDIR),native)
#	$V$(LD) -g -o $@.tmp.o -r $(OBJ) $(LIBGCC_PATH)
#	$V$(LD) -g $(LDFLAGS) -o $@ $@.tmp.o -Map $(_OBJPREFIX)Map.txt
#else
	$V$(LD) -g $(LDFLAGS) -o $@ $(OBJ) -Map $(_OBJPREFIX)Map.txt $(LIBGCC_PATH)
#endif
	$V$(DISASM) $(_BIN) > $(_OBJPREFIX)$(BIN).dsm

$(OBJ): $(_OBJPREFIX)%.o: %.c
	@echo [CC] -o $@
ifneq ($(_OBJPREFIX),)
	@mkdir -p $(dir $@)
endif
	$V$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@
	$V$(CC) -M -MP -MT $@ $(CPPFLAGS) $< -o $(_OBJPREFIX)$*.dep

$(OUTPUTDIR)Libs/libld-acess.so:
	@make -C $(ACESSDIR)/Usermode/Libraries/ld-acess.so_src/
$(OUTPUTDIR)Libs/%:
	@make -C $(ACESSDIR)/Usermode/Libraries/$*_src/

-include $(DEPFILES)

# vim: ft=make

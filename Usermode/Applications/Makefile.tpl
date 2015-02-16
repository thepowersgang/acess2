#
# Acess2
# - Application Template Makefile
#

CFLAGS   += -g
CXXFLAGS += -g
LDFLAGS  += -g

_BIN := $(OUTPUTDIR)$(DIR)/$(BIN)
_OBJPREFIX := obj-$(ARCH)/

LDFLAGS += -Map $(_OBJPREFIX)Map.txt

comma=,
LDFLAGS := $(subst -rpath-link ,-Wl$(comma)-rpath-link$(comma),$(LDFLAGS))
LDFLAGS := $(subst -Map ,-Wl$(comma)-Map$(comma),$(LDFLAGS))

_LIBS := $(filter -l%,$(LIBS))
_LIBS := $(patsubst -l%,$(OUTPUTDIR)Libs/lib%.so,$(_LIBS))

ifeq ($(ARCHDIR),native)
 LDFLAGS := $(patsubst -lc++,-lc++_acess,$(LDFLAGS))
 LIBS := $(patsubst -lc++,-lc++_acess,$(LIBS))
endif
ifeq ($(VERBOSE),)
V := @
else
V :=
endif

OBJ := $(addprefix $(_OBJPREFIX),$(OBJ))

#LINK_OBJS := $(CRTI) $(CRTBEGIN) $(CRT0) $(OBJ) $(LIBGCC_PATH) $(CRTEND) $(CRTN)
LINK_OBJS := $(OBJ)

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

$(_BIN): $(_LIBS) $(LINK_OBJS) $(CRT0) $(CRTI) $(CRTN)
	@mkdir -p $(dir $(_BIN))
	@echo [LD] -o $@
ifneq ($(USE_CXX_LINK),)
	$V$(CXX) -g $(LDFLAGS) -o $(_BIN) $(LINK_OBJS) $(LIBS)
else
	$V$(CC)  -g $(LDFLAGS) -o $(_BIN) $(LINK_OBJS) $(LIBS)
endif
	$V$(DISASM) $(_BIN) > $(_OBJPREFIX)$(BIN).dsm

$(_OBJPREFIX)%.o: %.c
	@echo [CC] -o $@
ifneq ($(_OBJPREFIX),)
	@mkdir -p $(dir $@)
endif
	$V$(CC)  $(CFLAGS)   $(CPPFLAGS) -c $< -o $@ -MQ $@ -MP -MD -MF $(_OBJPREFIX)$*.dep

$(_OBJPREFIX)%.o: %.cpp
	@echo [CXX] -o $@
ifneq ($(_OBJPREFIX),)
	@mkdir -p $(dir $@)
endif
	$V$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@ -MQ $@ -MP -MD -MF $(_OBJPREFIX)$*.dep

$(OUTPUTDIR)Libs/libld-acess.so:
	@make -C $(ACESSDIR)/Usermode/Libraries/ld-acess.so_src/
$(OUTPUTDIR)Libs/%:
	@make -C $(ACESSDIR)/Usermode/Libraries/$*_src/

-include $(DEPFILES)

# vim: ft=make

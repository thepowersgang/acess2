#
# Acess2 GUI (AxWin3)
# - By John Hodge (thePowersGang)
#
# sdl.mk
# - SDL-backed edition core definitions
#

UMODEBASE := ../../../
LIBBASE := $(UMODEBASE)Libraries/
LIBOUTDIR := $(UMODEBASE)Output/host/Libs

CPPFLAGS += -DAXWIN_SDL_BUILD=1
CPPFLAGS += $(patsubst -l%,-I$(LIBBASE)lib%.so_src/include_exp,$(filter -l%,$(LDFLAGS)))
CPPFLAGS += -I$(LIBBASE)libaxwin3.so_src/include_exp
LIBS := $(patsubst -l%,$(LIBBASE)lib%.so_src,$(filter -l%,$(LDFLAGS)))
LIBS := $(wildcard $(LIBS))
LIBS := $(LIBS:$(LIBBASE)%_src=$(LIBOUTDIR)%)
LDFLAGS := `sdl-config --libs` -L$(LIBOUTDIR) $(LDFLAGS)

ifeq ($(OS),Windows_NT)
BINSUFFIX := .exe
MKDIR := mkdir
RM := del /f /s /q
else
BINSUFFIX := 
MKDIR := mkdir -p
RM := rm -rf
endif

BDIR := obj-sdl/
_OBJ := $(OBJ:%=$(BDIR)%)
_BIN := ../bin-SDL/$(BIN)$(BINSUFFIX)

.PHONY: all clean

all: $(_BIN)

clean:
	$(RM) $(_BIN) $(_OBJ) $(BDIR)

$(_BIN): $(_OBJ) $(LIBS)
	@$(MKDIR) $(dir $@)
	@echo [LINK] $@
	@$(CC) $(LDFLAGS) -o $@ $(_OBJ)

$(BDIR)%.o: %.c
	@$(MKDIR) $(dir $@)
	@echo [CC] $@
	@$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ -c $<

$(LIBOUTDIR)lib%.so:
	-ARCH=host HOST_ARCH=x86_64 make -C $(LIBBASE)lib$*.so_src/ all


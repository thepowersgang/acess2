# 
# Acess2 OS - "Externals"
# - By John Hodge (thePowersGang)
#
# core.mk
# - Makefile code used by all externals

include $(dir $(lastword $(MAKEFILE_LIST)))/config.mk

#
# DEPS : Dependencies for this program/library 
# TARBALL_PATTERN : $(wildcard ) format [glob] pattern
# TARBALL_TO_DIR : $(patsubst ) format conversion
# PATCHES : List of altered files in the source
#  > If patches/%.patch exists, the original is patched, else patches/% is copied in
# [?]BTARGETS : Build targets (Defaults to all)
# [?]ARCHIVE : Optional forced archive (Defaults to latest)
# [?]NOBDIR : Set to non-empty to disable use of a separate build dir

# CONFIGURE_ARGS : Extra arguments to ./configure (ignoring host and prefix)
# [?]CONFIGURE_LINE : Command to create makefile (defaults to autotools configure)
# [?]AUTORECONF : Set to non-empty to regenerate ./configure


BTARGETS ?= all
ITARGETS ?= install

_VERS := $(wildcard $(TARBALL_PATTERN))
_VERS := $(sort $(_VERS))
_LATEST := $(lastword $(_VERS))

ifeq ($(ARCHIVE),)
 ifeq ($(_LATEST),)
  $(warning Unable to find an archive matching $(TARBALL_PATTERN))
  $(error No archive found)
 endif
 
 ifneq ($(_LATEST),$(_VERS))
  $(warning Multiple archvies found, picked $(_LATEST))
 endif
 ARCHIVE := $(_LATEST)
endif

DIR := $(patsubst $(TARBALL_TO_DIR_L),$(TARBALL_TO_DIR_R),$(ARCHIVE))

ifeq ($(NOBDIR),)
 BDIR := build-$(DIR)
else
 BDIR := $(DIR)
endif
SDIR := ../$(DIR)

.PHONY: all clean install _patch _build

all: $(DIR) _patch _build

clean:
	rm -rf $(DIR) $(BDIR)

$(DIR): $(ARCHIVE) patches/UNIFIED.patch
	tar -xf $(ARCHIVE)
ifneq ($(wildcard patches/UNIFIED.patch),)
	cd $(DIR) && patch -p1 < ../patches/UNIFIED.patch
endif

patches/UNIFIED.patch:
	

$(DIR)/%: patches/%.patch
	@echo [PATCH] $@
	@tar -xf $(ARCHIVE) $@
	@patch $@ $<

$(DIR)/%: patches/%
	@echo [CP] $@
	@mkdir -p $(dir $@)
	@cp $< $@

PATCHED_FILES := $(addprefix $(DIR)/,$(PATCHES))
_patch: $(DIR) $(PATCHED_FILES)

# 
# Acess2 OS - "Externals"
# - By John Hodge (thePowersGang)
#
# common.mk
# - Common makefile code for imported code

-include ../../Makefile.cfg

ifeq ($(ARCH),x86)
 BFD := i586
else ifeq ($(ARCH),x86_64)
 BFD := x86_64
else
 $(error No BFD translation for $(ARCH) in Externals/common.mk)
endif

#PREFIX=$(ACESSDIR)/Externals/Output
#EPREFIX=$(ACESSDIR)/Externals/Output/$(BFD)
PREFIX=$(ACESSDIR)/Externals/Output/$(ARCH)
EPREFIX=$(PREFIX)
SYSROOT=$(ACESSDIR)/Externals/Output/sysroot-$(BFD)
HOST=$(BFD)-acess_proxy-elf

#
# DEPS : Dependencies for this program/library 
# TARBALL_PATTERN : $(wildcard ) format [glob] pattern
# TARBALL_TO_DIR : $(patsubst ) format conversion
# PATCHES : List of altered files in the source
#  > If patches/%.patch exists, the original is patched, else patches/% is copied in
# CONFIGURE_ARGS : Extra arguments to ./configure (ignoring host and prefix)
# [?]BTARGETS : Build targets (Defaults to all)
# [?]ARCHIVE : Optional forced archive (Defaults to latest)
# [?]CONFIGURE_LINE : Command to create makefile (defaults to autotools configure)
# [?]NOBDIR : Set to non-empty to disable use of a separate build dir
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

CONFIGURE_LINE ?= $(SDIR)/configure --host=$(HOST) --prefix=$(PREFIX) --exec-prefix=$(EPREFIX) $(CONFIGURE_ARGS)

.PHONY: all clean install _patch _build

install: all
	cd $(BDIR) && make $(ITARGETS)

all: $(DIR) _patch _build

clean:
	rm -rf $(DIR) $(BDIR)

$(DIR): $(ARCHIVE)
	tar -xf $(ARCHIVE)

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

CONFIGSCRIPT := $(BDIR)/$(firstword $(CONFIGURE_LINE))
PATCHED_ACFILES := $(filter %/configure.in %/config.sub, $(PATCHED_FILES))
$(warning $(CONFIGSCRIPT): $(PATCHED_ACFILES))

$(CONFIGSCRIPT): $(PATCHED_ACFILES)
ifeq ($(AUTORECONF),)
else
	cd $(DIR) && autoreconf --force --install
endif

$(BDIR)/Makefile: _patch $(CONFIGSCRIPT) ../common.mk Makefile 
	mkdir -p $(BDIR)
	cd $(BDIR) && $(CONFIGURE_LINE)

_build: $(BDIR)/Makefile
	cd $(BDIR) && make $(BTARGETS)


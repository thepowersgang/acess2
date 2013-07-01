# 
# Acess2 OS - "Externals"
# - By John Hodge (thePowersGang)
#
# common.mk
# - Common makefile code for many autoconf(-like) externals

include $(dir $(lastword $(MAKEFILE_LIST)))/core.mk

CONFIGURE_LINE ?= $(SDIR)/configure --host=$(HOST) --prefix=$(PREFIX) --exec-prefix=$(EPREFIX) $(CONFIGURE_ARGS)


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
	cd $(BDIR) && PATH=$(PREFIX)-BUILD/bin:$(PATH) $(CONFIGURE_LINE)

_build: $(BDIR)/Makefile
	cd $(BDIR) && make $(BTARGETS)

install: all
	cd $(BDIR) && make $(ITARGETS)


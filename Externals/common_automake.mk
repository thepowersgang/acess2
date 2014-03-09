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
#	cd $(DIR) && aclocal --force -I acinclude
#	cd $(DIR) && libtoolize --force
	cd $(DIR) && autoreconf --force --install $(AUTORECONF_ARGS)
endif

$(BDIR)/Makefile: _patch $(CONFIGSCRIPT) ../common_automake.mk Makefile 
	mkdir -p $(BDIR)
	cd $(BDIR) && $(CONFIGURE_ENV) PATH=$(PATH) $(CONFIGURE_LINE)

_build: $(BDIR)/Makefile
	PATH=$(PATH) make $(BTARGETS) -C $(BDIR)
	PATH=$(PATH) make DESTDIR=$(OUTDIR) $(ITARGETS) -C $(BDIR)


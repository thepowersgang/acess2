#
# Acess2 External: zlib
# - Makefile and patches by John Hodge (thePowersGang)
#

DEPS := 
TARBALL_PATTERN := zlib-*.tar.gz
TARBALL_TO_DIR_L := %.tar.gz
TARBALL_TO_DIR_R := %
PATCHES := 
CONFIGURE_LINE = CC=$(HOST)-gcc $(SDIR)/configure --static --prefix=$(PREFIX) --eprefix=$(EPREFIX)
NOBDIR = yes

include ../common.mk


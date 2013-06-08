#
# Acess2 External: dropbear
# - Makefile and patches by John Hodge (thePowersGang)
#

DEPS := zlib
TARBALL_PATTERN := dropbear-*.tar.gz
TARBALL_TO_DIR_L := %.tar.gz
TARBALL_TO_DIR_R := %
PATCHES := options.h
CONFIGURE_ARGS = --with-zlib=../../Output/$(ARCH) --disable-utmp --disable-wtmp --disable-lastlog

include ../common.mk


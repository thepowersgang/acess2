#
# Acess2 External: dropbear
# - Makefile and patches by John Hodge (thePowersGang)
#

DEPS := zlib
TARBALL_PATTERN := dropbear-*.tar.bz2
TARBALL_TO_DIR_L := %.tar.bz2
TARBALL_TO_DIR_R := %
PATCHES := options.h
CONFIGURE_ARGS = --with-zlib=$(PREFIX) --disable-utmp --disable-wtmp --disable-lastlog
BTARGETS = dbclient
ITARGETS = inst_dbclient

include ../common.mk


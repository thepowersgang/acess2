#
# Acess2 External: dropbear
# - Makefile and patches by John Hodge (thePowersGang)
#

DEPS := zlib
TARBALL_PATTERN := dropbear-*.tar.bz2
TARBALL_TO_DIR_L := %.tar.bz2
TARBALL_TO_DIR_R := %
PATCHES := options.h config.sub
CONFIGURE_ARGS = --with-zlib=$(PREFIX) --disable-utmp --disable-wtmp --disable-lastlog
CONFIGURE_ENV = LIBS=-lpsocket
BTARGETS = dbclient dropbear
ITARGETS = inst_dbclient inst_dropbear

include ../common_automake.mk


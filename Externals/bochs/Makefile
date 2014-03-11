#
# Acess2 Externals - bochs
# - Patches and Makefile by John Hodge (thePowersGang)
#

DEPS := sdl12 cross-compiler
TARBALL_PATTERN := bochs-*.tar.gz
TARBALL_TO_DIR_L := bochs-%.tar.gz
TARBALL_TO_DIR_R := bochs-%
PATCHES := config.h.in config.sub

#AUTORECONF = yes
CONFIGURE_ARGS := --with-sdl --without-x11 LDFLAGS=-lpsocket

include ../common_automake.mk



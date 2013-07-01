#
# Acess2 Externals - netsurf
# - Patches and Makefile by John Hodge (thePowersGang)
#

DEPS := sdl12
TARBALL_PATTERN := netsurf-*-full-src.tar.gz
TARBALL_TO_DIR_L := netsurf-%-full-src.tar.gz
TARBALL_TO_DIR_R := netsurf-full-%
PATCHES := 
NOBDIR = yes

include ../core.mk

_build:
	cd $(BDIR) && CC=$(HOST)-gcc TARGET=sdl make



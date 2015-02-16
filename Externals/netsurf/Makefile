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

.PHONY: _check_local_deps

_build: _check_local_deps
	cd $(BDIR) && CC=$(HOST)-gcc TARGET=framebuffer make

_check_local_deps:
	@gperf --help >/dev/null || (echo "ERROR: netsurf's build system requires gperf"; false)



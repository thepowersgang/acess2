#
# Acess2 External: mesa
# - Makefile and patches by John Hodge (thePowersGang)
#

DEPS := 
TARBALL_PATTERN := MesaLib-*.tar.bz2
TARBALL_TO_DIR_L := MesaLib-%.tar.bz2
TARBALL_TO_DIR_R := Mesa-%
PATCHES := bin/config.sub
CONFIGURE_ARGS = --enable-osmesa --with-osmesa-bits=8 --disable-dri --disable-glx --with-dri-drivers=swrast --without-gallium-drivers --disable-egl --enable-shared

include ../common_automake.mk


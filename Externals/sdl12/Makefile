#
# Acess2 External: libsdl1.2
# - Makefile and patches by John Hodge (thePowersGang)
#

DEPS := 
TARBALL_PATTERN := SDL-*.tar.gz
TARBALL_TO_DIR_L := %.tar.gz
TARBALL_TO_DIR_R := %
PATCHES := configure build-scripts/config.sub
PATCHES += src/video/acess/ptyvideo.c src/video/SDL_video.c src/video/SDL_sysvideo.h
CONFIGURE_ARGS = --includedir=$(INCLUDEDIR) --bindir=$(PREFIX)-BUILD/bin/
#AUTORECONF = yes

include ../common_automake.mk


#
#
#
-include ../config.mk

GCC_ARCHIVE:=$(lastword $(sort $(wildcard gcc-*.tar.bz2)))
GCC_DIR:=$(GCC_ARCHIVE:%.tar.bz2=%)
BINUTILS_ARCHIVE:=$(lastword $(sort $(wildcard binutils-*.tar.bz2)))
BINUTILS_DIR:=$(BINUTILS_ARCHIVE:%.tar.bz2=%)

ifeq ($(GCC_ARCHIVE),)
 $(warning Unable to find a GCC archive matching gcc-*.tar.bz2)
 $(error No archive found)
endif
ifeq ($(BINUTILS_ARCHIVE),)
 $(warning Unable to find a binutils archive matching binutils-*.tar.bz2)
 $(error No archive found)
endif

BINUTILS_CHANGES := config.sub bfd/config.bfd gas/configure.tgt ld/configure.tgt ld/emulparams/acess2_i386.sh ld/emulparams/acess2_amd64.sh ld/emulparams/acess2_arm.sh ld/Makefile.in
GCC_CHANGES := config.sub gcc/config.gcc gcc/config/acess2.h libgcc/config.host gcc/config/acess2.opt
# libstdc++-v3/crossconfig.m4 config/override.m4

TARGET=$(HOST)
GCC_TARGETS := gcc target-libgcc
# target-libstdc++-v3 

#
# Acess2 Externals - glib
# - Patches and Makefile by John Hodge (thePowersGang)
#

DEPS := libffi
_NAME := glib
TARBALL_PATTERN := $(_NAME)-*.tar.xz
TARBALL_TO_DIR_L := $(_NAME)-%.tar.xz
TARBALL_TO_DIR_R := $(_NAME)-%
PATCHES := config.sub
CONFIGURE_ARGS := glib_cv_stack_grows=no ac_cv_func_posix_getpwuid_r=no ac_cv_func_posix_getgrgid_r=no
CONFIGURE_ARGS += LDFLAGS=-lpsocket

include ../common_automake.mk



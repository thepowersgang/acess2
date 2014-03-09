#
# Acess2 External: dropbear
# - Makefile and patches by John Hodge (thePowersGang)
#

DEPS := zlib
TARBALL_PATTERN := dropbear-*.tar.bz2
TARBALL_TO_DIR_L := %.tar.bz2
TARBALL_TO_DIR_R := %
PATCHES := options.h config.sub
CONFIGURE_ARGS = --with-zlib=$(OUTDIR) --disable-utmp --disable-wtmp --disable-lastlog DSS_PRIV_FILENAME=/Acess/Conf/dropbear/host_key_dss RSA_PRIV_FILENAME=/Acess/Conf/dropbear/host_key_rsa MOTD_FILENAME=/Acess/Conf/dropbear/motd
CONFIGURE_ENV = LIBS=-lpsocket
BTARGETS = dbclient dropbear
ITARGETS = inst_dbclient inst_dropbear

include ../common_automake.mk


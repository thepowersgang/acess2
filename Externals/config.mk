#
#
#
-include ../../Makefile.cfg

ifeq ($(ARCH),x86)
 BFD := i686
else ifeq ($(ARCH),x86_64)
 BFD := x86_64
else ifeq ($(ARCH),armv7)
 BFD := arm
else
 $(error No BFD translation for $(ARCH) in Externals/config.mk)
endif

OUTDIR=$(ACESSDIR)/Externals/Output/$(ARCH)
BUILD_OUTDIR=$(OUTDIR)-BUILD
SYSROOT=$(ACESSDIR)/Externals/Output/sysroot-$(BFD)

PATH:=$(BUILD_OUTDIR)/bin:$(PATH)
INCLUDE_DIR=$(SYSROOT)/usr/include

# Runtime Options
PREFIX=/Acess/usr/
EPREFIX=$(PREFIX)
HOST=$(BFD)-pc-acess2

PARLEVEL ?= 1


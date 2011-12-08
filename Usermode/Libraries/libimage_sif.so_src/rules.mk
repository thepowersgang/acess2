# libimage_sif
include $(BASE)header.mk

# Variables
SRCS := main.c
BIN  := $(OUTPUTDIR)Libs/libimage_sif.so

CFLAGS-$(DIR) := $(CFLAGS-$(PDIR))
CPPFLAGS-$(DIR) := $(CPPFLAGS-$(PDIR))
LDFLAGS-$(DIR) := $(LDFLAGS-$(PDIR)) -lc

include $(BASE)body.mk

$(BIN): $(OBJ) $(OUTPUTDIR)Libs/libc.so

include $(BASE)footer.mk


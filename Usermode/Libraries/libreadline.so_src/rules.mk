# libreadline
include $(BASE)header.mk

# Variables
SRCS := main.c
BIN  := $(OUTPUTDIR)Libs/libreadline.so

CFLAGS-$(DIR) := $(CFLAGS-$(PDIR))
CPPFLAGS-$(DIR) := $(CPPFLAGS-$(PDIR))
LDFLAGS-$(DIR) := $(LDFLAGS-$(PDIR)) -lc

include $(BASE)body.mk

$(BIN): $(OBJ) $(OUTPUTDIR)Libs/libc.so

include $(BASE)footer.mk


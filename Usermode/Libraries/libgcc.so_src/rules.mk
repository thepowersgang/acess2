# libgcc
include $(BASE)header.mk

# Variables
SRCS := libgcc.c
BIN  := $(OUTPUTDIR)Libs/libgcc.so

CFLAGS-$(DIR) := $(CFLAGS-$(PDIR))
CPPFLAGS-$(DIR) := $(CPPFLAGS-$(PDIR))
LDFLAGS-$(DIR) := $(LDFLAGS-$(PDIR))

include $(BASE)body.mk

$(BIN): $(OBJ)

include $(BASE)footer.mk

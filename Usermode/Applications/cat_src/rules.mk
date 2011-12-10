# cat

include $(BASE)header.mk

# Variables
SRCS := main.c
BIN  := $(OUTPUTDIR)Bin/cat

CFLAGS-$(DIR) := $(CFLAGS-$(PDIR)) -g
CPPFLAGS-$(DIR) := $(CPPFLAGS-$(PDIR))
LDFLAGS-$(DIR) := $(LDFLAGS-$(PDIR))

include $(BASE)body.mk

include $(BASE)footer.mk


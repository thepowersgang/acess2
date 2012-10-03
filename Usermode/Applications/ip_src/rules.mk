# ifconfig

include $(BASE)header.mk

# Variables
SRCS := main.c addr.c routes.c
BIN  := $(OUTPUTDIR)Bin/ifconfig

LDFLAGS-$(DIR) += -lnet

include $(BASE)body.mk

include $(BASE)footer.mk


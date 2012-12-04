# libc

include $(BASE)header.mk

# Variables
SRCS := stub.c heap.c stdlib.c env.c fileIO.c string.c select.c
SRCS += perror.c
SRCS += arch/$(ARCHDIR).$(ASSUFFIX)
# signals.c
BIN  := $(OUTPUTDIR)Libs/libc.so

CFLAGS-$(DIR) := $(CFLAGS-$(PDIR)) -g
CPPFLAGS-$(DIR) := $(CPPFLAGS-$(PDIR))
LDFLAGS-$(DIR) := $(LDFLAGS-$(PDIR)) -lgcc

include $(BASE)body.mk

include $(BASE)footer.mk

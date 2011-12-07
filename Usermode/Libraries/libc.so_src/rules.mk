# libc

include $(BASE)header.mk

# Variables
SRCS := stub.c heap.c stdlib.c env.c fileIO.c string.c select.c
SRCS += arch/$(ARCHDIR).$(ASSUFFIX)
# signals.c
BIN  := $(OUTPUTDIR)Libs/libc.so

CFLAGS-$(DIR) := $(CFLAGS-$(PDIR)) -g
CPPFLAGS-$(DIR) := $(CPPFLAGS-$(PDIR))
LDFLAGS-$(DIR) := $(LDFLAGS-$(PDIR))

include $(BASE)body.mk

$(BIN): $(OBJ) $(OUTPUTDIR)Libs/libgcc.so

include $(BASE)footer.mk

# CLIShell

include $(BASE)header.mk

# Variables
SRCS := main.c lib.c
BIN  := $(OUTPUTDIR)Bin/CLIShell

CFLAGS-$(DIR) := $(CFLAGS-$(PDIR)) -g
CPPFLAGS-$(DIR) := $(CPPFLAGS-$(PDIR)) -I$(DIR)/include
LDFLAGS-$(DIR) := $(LDFLAGS-$(PDIR)) -lreadline

include $(BASE)body.mk

$(BIN): $(OBJ)

include $(BASE)footer.mk


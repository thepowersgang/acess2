# crt0 and friends

include $(BASE)header.mk

# Variables
SRCS := crt0.c crtbegin.c crtend.c
# signals.c
BIN := $(OUTPUTDIR)Libs/crt0.o $(OUTPUTDIR)Libs/crtbegin.o $(OUTPUTDIR)Libs/crtend.o

CFLAGS-$(DIR) := $(CFLAGS-$(PDIR)) -g
CPPFLAGS-$(DIR) := $(CPPFLAGS-$(PDIR))
LDFLAGS-$(DIR) := $(LDFLAGS-$(PDIR))

include $(BASE)body.mk

$(filter %crt0.o,$(BIN)): $(filter %crt0.c.o,$(OBJ))
	cp $< $@
$(filter %crtbegin.o,$(BIN)): $(filter %crtbegin.c.o,$(OBJ))
	cp $< $@
$(filter %crtend.o,$(BIN)): $(filter %crtend.c.o,$(OBJ))
	cp $< $@

include $(BASE)footer.mk

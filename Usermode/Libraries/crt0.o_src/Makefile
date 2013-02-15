#
#
#

-include ../Makefile.cfg

BIN = $(OUTPUTDIR)Libs/crt0.o $(OUTPUTDIR)Libs/crtbegin.o $(OUTPUTDIR)Libs/crtend.o

.PHONY: all clean install utest generate_exp

all: $(BIN)

install: $(BIN)

clean:
	$(RM) $(BIN)

# Disabled unit tests
utest generate_exp:
	@echo > /dev/null

$(OUTPUTDIR)Libs/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) -c $< -o $@

#$(OUTPUTDIR)Libs/crt0.o: obj-$(ARCH)/crt0_asm.o obj-$(ARCH)/crt0_c.o
#	@mkdir -p $(dir $@)
#	$(LD) -r -o $@ $?

#obj-$(ARCH)/crt0_asm.o: crt0.$(ARCHDIR).$(ASSUFFIX)
#	@mkdir -p $(dir $@)
#	$(AS) $(ASFLAGS) $< -o $@

#obj-$(ARCH)/crt0_c.o: crt0.c
#	@mkdir -p $(dir $@)
#	$(CC) -c $< -o $@


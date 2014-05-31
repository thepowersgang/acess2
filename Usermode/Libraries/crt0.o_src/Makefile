#
#
#

-include ../Makefile.cfg

BIN = $(OUTPUTDIR)Libs/crt0.o $(OUTPUTDIR)Libs/crt0S.o $(OUTPUTDIR)Libs/crti.o $(OUTPUTDIR)Libs/crtn.o

CFLAGS := -std=c99

.PHONY: all clean install utest utest-build generate_exp

all: $(BIN)

install: $(BIN)

clean:
	$(RM) $(BIN)

# Disabled unit tests
utest generate_exp utest-build utest-run:
	@echo > /dev/null

$(OUTPUTDIR)Libs/%.o: %.c Makefile
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@
$(OUTPUTDIR)Libs/%S.o: %S.c Makefile
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@ -fPIC
$(OUTPUTDIR)Libs/%.o: $(ARCHDIR)-%.S
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

#$(OUTPUTDIR)Libs/crt0.o: obj-$(ARCH)/crt0_asm.o obj-$(ARCH)/crt0_c.o
#	@mkdir -p $(dir $@)
#	$(LD) -r -o $@ $?

#obj-$(ARCH)/crt0_asm.o: crt0.$(ARCHDIR).$(ASSUFFIX)
#	@mkdir -p $(dir $@)
#	$(AS) $(ASFLAGS) $< -o $@

#obj-$(ARCH)/crt0_c.o: crt0.c
#	@mkdir -p $(dir $@)
#	$(CC) -c $< -o $@


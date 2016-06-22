# InitRD Filesystem Driver
#

OBJ = main.o files.$(ARCH).o
EXTRA = files.c
NAME = InitRD
EXTRA = files.$(ARCH).c files.$(ARCH).c.dep files.$(ARCH).c.ldopts
LDFLAGS += @files.$(ARCH).c.ldopts

-include ../Makefile.tpl


files.$(ARCH).c: generate_initrd files.lst
	@echo "--- generate_initrd files.lst $@"
	$VARCH=$(ARCH) ./generate_initrd files.lst $@ $@.ldopts $@.dep


generate_initrd: generate_initrd.c
	@echo "--- [NCC] -o $@"
	$V$(NCC) -std=c11 -o $@ $< -g -Wall -Wextra

# Override default install method, because the floppy is small :)
install:
	true

-include files.$(ARCH).c.dep

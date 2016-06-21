# InitRD Filesystem Driver
#

OBJ = main.o files.$(ARCH).o
EXTRA = files.c
NAME = InitRD
EXTRA = files.$(ARCH).c files.$(ARCH).c.dep files.$(ARCH).c.ldopts
LDFLAGS += @files.$(ARCH).c.ldopts

-include ../Makefile.tpl


files.$(ARCH).c: GenerateInitRD.php generate_initrd files.lst
	#ARCH=$(ARCH) ACESSDIR=$(ACESSDIR) php GenerateInitRD.php files.lst $@ $@.ldopts $@.dep
	ARCH=$(ARCH) ./generate_initrd files.lst $@ $@.ldopts $@.dep


generate_initrd: generate_initrd.c
	$(NCC) -std=c11 -o $@ $< -g -Wall -Wextra

# Override default install method, because the floppy is small :)
install:
	true

-include files.$(ARCH).c.dep

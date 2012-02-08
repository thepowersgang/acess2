# InitRD Filesystem Driver
#

OBJ = main.o files.$(ARCH).o
EXTRA = files.c
NAME = InitRD
EXTRA = files.$(ARCH).c files.$(ARCH).c.dep files.$(ARCH).c.ldopts
LDFLAGS += @files.$(ARCH).c.ldopts

-include ../Makefile.tpl


files.$(ARCH).c: GenerateInitRD.php files.lst
	ARCH=$(ARCH) ACESSDIR=$(ACESSDIR) php GenerateInitRD.php files.lst $@ $@.ldopts $@.dep

-include files.$(ARCH).c.dep

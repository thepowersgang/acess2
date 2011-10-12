# InitRD Filesystem Driver
#

OBJ = main.o files.$(ARCH).o
EXTRA = files.c
NAME = InitRD
EXTRA = files.$(ARCH).c files.$(ARCH).c.dep

-include ../Makefile.tpl


files.$(ARCH).c: GenerateInitRD.php files.lst
	ARCH=$(ARCH) ACESSDIR=$(ACESSDIR) php GenerateInitRD.php files.lst $@ $@.dep

-include files.$(ARCH).c.dep

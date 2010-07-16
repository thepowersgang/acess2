# InitRD Filesystem Driver
#

OBJ = main.o files.o
EXTRA = files.c
NAME = InitRD

-include ../Makefile.tpl

files.c: GenerateInitRD.php files.lst
	php GenerateInitRD.php files.lst files.c files.c.d

-include files.c.d

# InitRD Filesystem Driver
#

OBJ = main.o files.o
NAME = FS_InitRD

-include ../Makefile.tpl

files.c: GenerateInitRD.php files.lst
	php GenerateInitRD.php files.lst files.c

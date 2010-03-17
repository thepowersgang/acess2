# InitRD Filesystem Driver
#

OBJ = main.o files.o
NAME = FS_InitRD

-include ../Makefile.tpl

files.c:
	php GenerateInitRD.php files.lst > files.c

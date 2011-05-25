# Project: text editor

-include ../Makefile.cfg

OBJ = main.o
BIN = edit

-include ../Makefile.tpl

edit.lin: main.c
	gcc main.c -o edit.lin -DUSE_LOCAL=1 -Wall


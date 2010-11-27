# Acess Dynamic Linker (ELF) Version 1
#  LD-ACESS.SO
#  Makefile

-include ../Makefile.cfg

OBJ := main.o lib.o loadlib.o elf.o pe.o
OBJ += core.ao vfs.ao mm.ao
BIN = ld-acess.so

CFLAGS   = -Wall -fno-builtin -fno-leading-underscore -fno-stack-protector
ASFLAGS  = -felf
LDFLAGS  = -T link.ld -Map map.txt -Bstatic

include ../Makefile.tpl


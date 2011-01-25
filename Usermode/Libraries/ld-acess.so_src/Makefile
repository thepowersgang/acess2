# Acess Dynamic Linker (ELF) Version 1
#  LD-ACESS.SO
#  Makefile

-include ../Makefile.cfg

OBJ := main.o lib.o loadlib.o export.o elf.o pe.o
OBJ += core.ao vfs.ao mm.ao
BIN = ld-acess.so
EXTRABIN := libld-acess.so

CFLAGS   = -Wall -fno-builtin -fno-leading-underscore -fno-stack-protector
CFLAGS  += $(CPPFLAGS)
ASFLAGS  = -felf
LDFLAGS  = -T link.ld -Map map.txt --export-dynamic

include ../Makefile.tpl

$(_XBIN): $(_BIN)
	@echo [LD] -o -shared libld-acess.so
	@$(LD) $(LDFLAGS) -shared -o $@ $(OBJ)


# Acess Dynamic Linker (ELF) Version 1
#  LD-ACESS.SO
#  Makefile

CC = gcc
AS = nasm
RM = @rm -f
LD = ld
OBJDUMP = objdump

COBJ = main.o lib.o loadlib.o elf.o pe.o
AOBJ = helpers.ao
BIN = ../ld-acess.so

CPPFLAGS = -I../../include
CFLAGS = -Wall -fno-builtin -fleading-underscore -fno-stack-protector
ASFLAGS = -felf
#LDFLAGS = --oformat elf32-i386 -Map map.txt -Bstatic -e _SoMain -shared
#LDFLAGS = --oformat elf32-i386 -Map map.txt -Bstatic -e _SoMain -Ttext 0xBFFFE000
LDFLAGS = -T link.ld -Map map.txt -Bstatic


.PHONY: all clean

all:	$(BIN)

clean:
	$(RM) $(BIN) $(AOBJ) $(COBJ)

$(BIN): $(AOBJ) $(COBJ)
	$(LD) $(LDFLAGS) -o $(BIN) $(AOBJ) $(COBJ) > link.txt
	$(OBJDUMP) -x $(BIN) > ld-acess.dmp
	$(OBJDUMP) -d $(BIN) > ld-acess.dsm
	cp $(BIN) /mnt/AcessHDD/Acess2/Libs

$(COBJ): %.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

$(AOBJ): %.ao: %.asm
	$(AS) $(ASFLAGS) -o $@ $<
	

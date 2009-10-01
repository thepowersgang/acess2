# Project: Acess Shell

CC = gcc
AS = nasm
LD = ld
RM = @rm -f

COBJ = main.o lib.o
BIN = ../CLIShell
ACESSDIR = /home/hodgeja/Projects/Acess2/Usermode

INCS = -I$(ACESSDIR)/include -I./include
CFLAGS = -Wall -fno-builtin -fno-stack-protector $(INCS)
ASFLAGS = -felf
LDFLAGS = -T $(ACESSDIR)/Libraries/acess.ld -I /Acess/Libs/ld-acess.so -lc

.PHONY : all clean

all: $(BIN)

$(BIN): $(AOBJ) $(COBJ)
	@echo --- $(LD) -o $@
	@$(LD) $(LDFLAGS) -o $@ $(AOBJ) $(COBJ) -Map Map.txt
	objdump -d $(BIN) > $(BIN).dsm
	cp $(BIN) /mnt/AcessHDD/Acess2/Bin/

clean:
	$(RM) $(AOBJ) $(COBJ) $(BIN)

$(COBJ): %.o: %.c
	@echo --- GCC -o $@
	@$(CC) $(CFLAGS) -c $? -o $@

$(AOBJ): %.ao: %.asm
	@echo --- $(AS) -o $@
	@$(AS) $(ASFLAGS) -o $@ $<

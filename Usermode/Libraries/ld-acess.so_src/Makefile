# Acess Dynamic Linker (ELF) Version 1
#  LD-ACESS.SO
#  Makefile

-include ../Makefile.cfg

COBJ = main.o lib.o loadlib.o elf.o pe.o
AOBJ = helpers.ao
BIN = ../ld-acess.so

CFLAGS   = -Wall -fno-builtin -fleading-underscore -fno-stack-protector
ASFLAGS  = -felf
LDFLAGS  = -T link.ld -Map map.txt -Bstatic


.PHONY: all clean

all:	$(BIN)

clean:
	$(RM) $(BIN) $(AOBJ) $(COBJ)

$(BIN): $(AOBJ) $(COBJ)
	@echo --- $(LD) -shared -o $@
	@$(LD) $(LDFLAGS) -o $(BIN) $(AOBJ) $(COBJ) > link.txt
	$(OBJDUMP) -x $(BIN) > ld-acess.dmp
	$(OBJDUMP) -d $(BIN) > ld-acess.dsm
	cp $(BIN) $(DISTROOT)/Libs

$(COBJ): %.o: %.c
	@echo $(CC) -o $@
	@$(CC) $(CFLAGS) -o $@ -c $<

$(AOBJ): %.ao: %.asm
	@echo $(AS) -o $@
	@$(AS) $(ASFLAGS) -o $@ $<
	

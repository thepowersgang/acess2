# Acess Dynamic Linker (ELF) Version 1
#  LD-ACESS.SO
#  Makefile

-include ../Makefile.cfg

COBJ = main.o lib.o loadlib.o elf.o pe.o
AOBJ = core.ao vfs.ao mm.ao
BIN = ../ld-acess.so

CFLAGS   = -Wall -fno-builtin -fno-leading-underscore -fno-stack-protector
ASFLAGS  = -felf
LDFLAGS  = -T link.ld -Map map.txt -Bstatic


.PHONY: all clean install

all:	$(BIN)

clean:
	$(RM) $(BIN) $(AOBJ) $(COBJ) ld-acess.dmp ld-acess.dsm link.txt map.txt

install: $(BIN)
	$(xCP) $(BIN) $(DISTROOT)/Libs/

$(BIN): $(AOBJ) $(COBJ)
	@echo --- $(LD) -shared -o $@
	@$(LD) $(LDFLAGS) -o $(BIN) $(AOBJ) $(COBJ) > link.txt
	$(OBJDUMP) -x $(BIN) > ld-acess.dmp
	$(OBJDUMP) -d $(BIN) > ld-acess.dsm

$(COBJ): %.o: %.c
	@echo $(CC) -o $@
	@$(CC) $(CFLAGS) -o $@ -c $<

$(AOBJ): %.ao: %.asm
	@echo $(AS) -o $@
	@$(AS) $(ASFLAGS) -o $@ $<
	

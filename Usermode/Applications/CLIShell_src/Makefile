# Project: Acess Shell

-include ../Makefile.cfg

CPPFLAGS += -I./include
CFLAGS   += -Wall -fno-builtin -fno-stack-protector
LDFLAGS  += 

BIN = ../CLIShell
COBJ = main.o lib.o

.PHONY : all clean

all: $(BIN)

clean:
	$(RM) $(COBJ) $(BIN) $(BIN).dsm Map.txt

$(BIN): $(COBJ)
	@echo --- $(LD) -o $@
	@$(LD) $(LDFLAGS) -o $@ $(COBJ) -Map Map.txt
	objdump -d $(BIN) > $(BIN).dsm
	cp $(BIN) $(DISTROOT)/Bin/

$(COBJ): %.o: %.c
	@echo --- GCC -o $@
	@$(CC) $(CFLAGS) -c $? -o $@

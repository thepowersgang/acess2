# Project: cat

-include ../Makefile.cfg

COBJ = main.o
BIN = ../cat

CFLAGS  += -Wall -fno-builtin -fno-stack-protector
LDFLAGS += 

.PHONY : all clean

all: $(BIN)

$(BIN): $(COBJ)
	@echo --- $(LD) -o $@
	@$(LD) $(LDFLAGS) -o $@ $(COBJ) -Map Map.txt
	objdump -d $(BIN) > $(BIN).dsm
	$(xCP) $(BIN) $(DISTROOT)/Bin/

clean:
	$(RM) $(COBJ) $(BIN) $(BIN).dsm Map.txt

$(COBJ): %.o: %.c
	@echo --- GCC -o $@
	@$(CC) $(CFLAGS) -c $? -o $@


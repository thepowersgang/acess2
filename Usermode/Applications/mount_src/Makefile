# Project: mount

-include ../Makefile.cfg

COBJ = main.o
BIN = ../mount

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
	$(RM) $(COBJ) $(BIN) Map.txt $(BIN).dsm

$(COBJ): %.o: %.c
	@echo --- GCC -o $@
	@$(CC) $(CFLAGS) -c $? -o $@


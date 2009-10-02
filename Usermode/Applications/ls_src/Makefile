# Project: cat

-include ../Makefile.cfg

COBJ = main.o
BIN = ../ls

CFLAGS  += -Wall -fno-builtin
LDFLAGS +=

.PHONY : all clean

all: $(BIN)

$(BIN): $(COBJ)
	@echo --- $(LD) -o $@
	@$(LD) $(LDFLAGS) -o $@ $(COBJ) -Map Map.txt
	objdump -d $(BIN) > $(BIN).dsm
	cp $(BIN) $(DISTROOT)/Bin/

clean:
	$(RM) $(COBJ) $(BIN)

$(COBJ): %.o: %.c
	@echo --- GCC -o $@
	@$(CC) $(CFLAGS) -c $? -o $@


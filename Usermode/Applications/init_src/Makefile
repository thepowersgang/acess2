#

-include ../Makefile.cfg

CPPFLAGS += 
CFLAGS	 += -Wall -Werror -O3
LDFLAGS  += 

BIN = ../init
OBJ = main.o

.PHONY: all clean

all: $(BIN)

clean:
	$(RM) $(BIN) $(OBJ) $(BIN).dsm

$(BIN): $(OBJ) Makefile
	@echo --- ld -o $(BIN)
	@$(LD) $(LDFLAGS) $(OBJ) -o $(BIN)
	$(OBJDUMP) -d $(BIN) > $(BIN).dsm
	cp $(BIN) $(DISTROOT)/SBin/

%.o: %.c
	@echo --- cc -o $@
	@$(CC) $(CFLAGS) -c $< -o $@

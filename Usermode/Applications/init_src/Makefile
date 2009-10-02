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
	$(RM) $(BIN) $(OBJ)

$(BIN): $(OBJ) Makefile
	$(LD) $(LDFLAGS) $(OBJ) -o $(BIN)
	cp $(BIN) $(DISTROOT)/SBin/

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

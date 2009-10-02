# Acess 2 Login Shell

-include ../Makefile.cfg

DBTYPE = tpl

CPPFLAGS += 
CFLAGS   += -fno-stack-protector -Wall -Werror -O3 $(CPPFLAGS)
LDFLAGS  += -lgcc -lc

OBJ = main.o database_$(DBTYPE).o
BIN = ../login

.PHONY: all clean

all: $(BIN)

clean:
	$(RM) $(BIN) $(OBJ)

$(BIN): $(OBJ)
	@echo --- $(LD) -o $@
	@$(LD) $(LDFLAGS) $(OBJ) -o $@
	cp $(BIN) $(DISTROOT)/SBin/
	objdump -d $(BIN) > login.dsm

%.o: %.c Makefile
	@echo --- $(CC) -o $@
	@$(CC) $(CFLAGS) -c $< -o $@

# Acess2 - Application Template Makefile
#
#

CFLAGS  += -Wall -fno-builtin -fno-stack-protector
LDFLAGS += 

.PHONY : all clean install

all: $(BIN)

clean:
	@$(RM) $(OBJ) $(BIN) $(BIN).dsm Map.txt

install: $(BIN)
	$(xCP) $(BIN) $(DISTROOT)/$(DIR)/

$(BIN): $(OBJ)
	@echo --- $(LD) -o $@
	@$(LD) $(LDFLAGS) -o $@ $(OBJ) -Map Map.txt
	@objdump -d $(BIN) > $(BIN).dsm

$(OBJ): %.o: %.c
	@echo --- GCC -o $@
	@$(CC) $(CFLAGS) $(CPPFLAGS) -c $? -o $@

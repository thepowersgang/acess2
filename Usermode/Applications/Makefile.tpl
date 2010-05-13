# Acess2 - Application Template Makefile
#
#

CFLAGS  += -Wall -fno-builtin -fno-stack-protector -g
LDFLAGS += 

DEPFILES := $(OBJ:%.o=%.d)

.PHONY : all clean install

all: $(BIN)

clean:
	@$(RM) $(OBJ) $(DEPFILES) $(BIN) $(BIN).dsm Map.txt

install: $(BIN)
	$(xCP) $(BIN) $(DISTROOT)/$(DIR)/

$(BIN): $(OBJ)
	@echo --- $(LD) -o $@
	@$(LD) -g $(LDFLAGS) -o $@ $(OBJ) -Map Map.txt
	@objdump -d -S $(BIN) > $(BIN).dsm

$(OBJ): %.o: %.c
	@echo --- GCC -o $@
	@$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@
	@$(CC) -M -MT $@ $(CPPFLAGS) $< -o $*.d

-include $(DEPFILES)

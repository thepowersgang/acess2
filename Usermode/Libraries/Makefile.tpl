# Acess 2 SQLite 3 Library
#

DEPFILES := $(OBJ:%.o=%.d)

.PHONY: all clean install

all: $(BIN)

clean:
	$(RM) $(BIN) $(OBJ) $(BIN).dsm

install: $(BIN)
	$(xCP) $(BIN) $(DISTROOT)/Libs/

$(BIN): $(OBJ)
	@echo [LD] -o $(BIN) $(OBJ)
	@$(LD) $(LDFLAGS) -o $(BIN) $(OBJ)
	@$(OBJDUMP) -d -S $(BIN) > $(BIN).dsm

%.o: %.c
	@echo [CC] -o $@
	@$(CC) $(CFLAGS) -o $@ -c $<
	@$(CC) -M -MT $@ $(CPPFLAGS) $< -o $*.d

-include $(DEPFILES)

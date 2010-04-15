# Acess 2 SQLite 3 Library
#

.PHONY: all clean install

all: $(BIN)

clean:
	$(RM) $(BIN) $(OBJ) $(BIN).dsm

install: $(BIN)
	$(xCP) $(BIN) $(DISTROOT)/Libs/

$(BIN): $(OBJ)
	$(LD) $(LDFLAGS) -o $(BIN) $(OBJ)
	@$(OBJDUMP) -d $(BIN) > $(BIN).dsm

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

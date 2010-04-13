# Acess 2 SQLite 3 Library
#

.PHONY: all clean install

all: $(BIN)

clean:
	$(RM) $(BIN) $(OBJ)

install: $(BIN)
	$(xCP) $(BIN) $(DISTROOT)/Libs/

$(BIN): $(OBJ)
	$(LD) $(LDFLAGS) -o $(BIN) $(OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

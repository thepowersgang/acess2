#
#

ifeq ($(PLATFORM),)
	PLATFORM := lin
endif

OBJ := syscalls.$(PLATFORM).o request.$(PLATFORM).o

ifeq ($(PLATFORM),win)
	BIN := ../libacess.dll
endif
ifeq ($(PLATFORM),lin)
	BIN := ../libacess.so
	CFLAGS += -fPIC
endif


.PHONY: all clean

all: $(BIN)

clean:
	$(RM) $(BIN) $(OBJ)

$(BIN): $(OBJ)
	$(CC) -shared -o $@ $<

%.$(PLATFORM).o: %.c
	$(CC) -c $< -o $@ $(CFLAGS) $(CPPFLAGS)


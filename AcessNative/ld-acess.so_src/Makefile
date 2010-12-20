#
#

ifeq ($(PLATFORM),)
	PLATFORM := lin
endif

OBJ := main.o syscalls.o request.o
OBJ += elf.o
OBJ := $(addsuffix .$(PLATFORM),$(OBJ))

ifeq ($(PLATFORM),win)
	BIN := ../ld-acess.exe
endif
ifeq ($(PLATFORM),lin)
	BIN := ../ld-acess
endif

CFLAGS += -Wall -Werror

.PHONY: all clean

all: $(BIN)

clean:
	$(RM) $(BIN) $(OBJ)

$(BIN): $(OBJ)
	$(CC) -shared -o $@ $<

%.o.$(PLATFORM): %.c
	$(CC) -c $< -o $@ $(CFLAGS) $(CPPFLAGS)


#
#

ifeq ($(PLATFORM),)
	PLATFORM := lin
endif

OBJ := main.o syscalls.o request.o binary.o memory.o
OBJ += elf.o
OBJ := $(addsuffix .$(PLATFORM),$(OBJ))

ifeq ($(PLATFORM),win)
	BIN := ../ld-acess.exe
endif
ifeq ($(PLATFORM),lin)
	BIN := ../ld-acess
	LD += -m elf_i386
endif

CFLAGS += -Wall -Werror -g -m32

.PHONY: all clean

all: $(BIN)

clean:
	$(RM) $(BIN) $(OBJ)

$(BIN): link.ld.$(PLATFORM) $(OBJ)
#	$(LD) -g -o $@ $(OBJ) -T link.ld.$(PLATFORM)
	$(CC) -g -o $@ $(OBJ) -m32 -Wl,-T,link.ld.$(PLATFORM)

%.o.$(PLATFORM): %.c
	$(CC) -c $< -o $@ $(CFLAGS) $(CPPFLAGS)

# Modify the default makefile to put the executable at 1MB instead
link.ld.lin:
	$(LD) --verbose | awk '{ if( substr($$0,0,5) == "====="){ bPrint = !bPrint; } else { if(bPrint){ print $$0;} } }' | sed 's/\b0x0[08][0-9]*\b/0x00100000/g' > $@


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

DEPFILES  = $(filter %.o.$(PLATFORM),$(OBJ))
DEPFILES := $(DEPFILES:%.o.$(PLATFORM)=%.d.$(PLATFORM))

.PHONY: all clean

all: $(BIN)

clean:
	$(RM) $(BIN) $(OBJ) $(DEPFILES)

$(BIN): link.ld.$(PLATFORM) $(OBJ)
#	$(LD) -g -o $@ $(OBJ) -T link.ld.$(PLATFORM)
	$(CC) -g -o $@ $(OBJ) -m32 -Wl,-T,link.ld.$(PLATFORM)

%.o.$(PLATFORM): %.c
	@echo [CC] -o $@
	@$(CC) -c $< -o $@ $(CFLAGS) $(CPPFLAGS)
	@$(CC) -M $(CPPFLAGS) -MT $@ -o $*.d.$(PLATFORM) $<

# Modify the default makefile to put the executable at 1MB instead
link.ld.lin:
	@echo Making Linker Script ($@)
	@$(LD) --verbose | awk '{ if( substr($$0,0,5) == "====="){ bPrint = !bPrint; } else { if(bPrint){ print $$0;} } }' | sed 's/\b0x0[08][0-9]*\b/0x00100000/g' > $@

-include $(DEPFILES)


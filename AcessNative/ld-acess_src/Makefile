#
#

ifeq ($(PLATFORM),)
	PLATFORM := lin
endif

OBJ := main.o syscalls.o request.o binary.o memory.o exports.o
OBJ += elf.o elf_load.o
OBJ := $(addprefix obj-$(PLATFORM)/,$(OBJ))

ifeq ($(PLATFORM),win)
	BIN := ../ld-acess.exe
endif
ifeq ($(PLATFORM),lin)
	BIN := ../ld-acess
	LINKADDR := 0x200000
#	LD += -m elf_i386
endif

CFLAGS   += -Wall
CFLAGS   += -Werror
CFLAGS   += -g
CPPFLAGS += -DARCHDIR_is_x86_64=1
LDFLAGS  += -g
# -Wl,-T,obj-$(PLATFORM)/link.ld

DEPFILES  = $(filter %.o,$(OBJ))
DEPFILES := $(DEPFILES:%=%.dep)

.PHONY: all clean

all: $(BIN)

clean:
	$(RM) $(BIN) $(OBJ) $(DEPFILES) obj-$(PLATFORM)/link.ld

$(BIN): obj-$(PLATFORM)/link.ld $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $(OBJ)
	objdump -S $@ > $@.dsm

obj-$(PLATFORM)/%.o: %.c
	@mkdir -p $(dir $@)
	@echo [CC] -o $@
	@$(CC) -c $< -o $@ $(CFLAGS) $(CPPFLAGS)
	@$(CC) -M $(CPPFLAGS) -MT $@ -o $@.dep $<

# Modify the default makefile to put the executable at 1MB instead
obj-lin/link.ld:
	@mkdir -p $(dir $@)
	@echo "Making Linker Script ($@)"
	$(LD) -g --verbose | awk '{ if( substr($$1,0,5) == "====="){ bPrint = !bPrint; } else { if(bPrint){ print $$0;} } }' | sed 's/\b0x[048][0-9]*\b/$(LINKADDR)/g' | sed 's/CONSTANT (MAXPAGESIZE)/0x1000/g' > $@

-include $(DEPFILES)


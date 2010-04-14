# Acess2 Basic C Library
# Makefile

-include ../Makefile.cfg

CPPFLAGS += 
CFLAGS   += 
ASFLAGS  +=
LDFLAGS  += -soname libc.so -Map map.txt -lgcc

OBJ = stub.o heap.o stdlib.o env.o fileIO.o string.o
DEPFILES := $(OBJ:%.o=%.d)
# signals.o
BIN = ../libc.so

.PHONY:	all clean install

all: $(BIN)

clean:
	$(RM) $(BIN) $(OBJ) $(DEPFILES) libc.so.dsm libc.so.dmp map.txt

install: $(BIN)
	$(xCP) $(BIN) $(DISTROOT)/Libs/

# Core C Library
$(BIN): $(OBJ)
	@echo --- ld -shared -o $@
	@$(LD) $(LDFLAGS) $(OBJ) -o $@
	@$(OBJDUMP) -d $@ > libc.so.1.dsm
	@$(OBJDUMP) -x -r -R $@ > libc.so.1.dmp

# C Runtime 0
../crt0.o: crt0.asm
	@echo --- $(AS) -o $@
	@$(AS) $(ASFLAGS) -o $@ $<

$(filter %.o, $(OBJ)): %.o: %.c
	@echo --- $(CC) -o $@
	@$(CC) $(CFLAGS) -DBUILD_SO -o $@ -c $<
	@$(MAKEDEP) $(CPPFLAGS) -MT $@ -o $*.d $<

$(filter %.ao, $(OBJ)): %.ao: %.asm
	@echo --- $(AS) -o $@
	@$(AS) $(ASFLAGS) -o $@ $<

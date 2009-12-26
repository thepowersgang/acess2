# Acess2 Basic C Library
# Makefile

-include ../Makefile.cfg

CPPFLAGS += 
CFLAGS   += 
ASFLAGS  +=
LDFLAGS  += -soname libc.so.1 -Map map.txt -lgcc

OBJ = stub.o heap.o stdlib.o env.o fileIO.o string.o
DEPFILES := $(OBJ:%.o=%.d)
# signals.o
BIN = ../libc.so.1

.PHONY:	all clean

all: $(BIN)

clean:
	$(RM) $(BIN) ../libc.so $(OBJ) $(DEPFILES) libc.so.1.dsm libc.so.1.dmp map.txt

# Core C Library
$(BIN): $(OBJ)
	@echo --- ld -shared -o $@
	@$(LD) $(LDFLAGS) $(OBJ) -o $@
	@$(OBJDUMP) -d $@ > libc.so.1.dsm
	@$(OBJDUMP) -x -r -R $@ > libc.so.1.dmp
	cp ../libc.so.1 ../libc.so
	$(xCP) ../libc.so.1 $(DISTROOT)/Libs/

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

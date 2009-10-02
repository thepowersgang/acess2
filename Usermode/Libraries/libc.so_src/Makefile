# Acess2 Basic C Library
# Makefile

-include ../Makefile.cfg

CPPFLAGS += 
CFLAGS   += 
ASFLAGS  +=
LDFLAGS  += -soname libc.so.1 -Map map.txt

OBJ_LIBC = heap.o stdlib.o stub.o env.o fileIO.o string.o
# signals.o
BIN = ../libc.so.1

.PHONY:	all clean

all: $(BIN) $(OBJ_LIBC)

clean:
	$(RM) $(BIN) $(OBJ_LIBC)

# Core C Library
$(BIN): $(OBJ_LIBC)
	@echo --- ld -shared -o $@
	@$(LD) $(LDFLAGS) $(OBJ_LIBC) -o $@
	$(OBJDUMP) -d $@ > libc.so.1.dsm
	$(OBJDUMP) -x -r -R $@ > libc.so.1.dmp
	cp ../libc.so.1 ../libc.so
	cp ../libc.so.1 $(DISTROOT)/Libs/

# C Runtime 0
../crt0.o: crt0.asm
	@echo --- $(AS) -o $@
	@$(AS) $(ASFLAGS) -o $@ $<

$(filter %.o, $(OBJ_LIBC)): %.o: %.c config.h
	@echo --- $(CC) -o $@
	@$(CC) $(CFLAGS) -DBUILD_SO -o $@ -c $<

$(filter %.ao, $(OBJ_LIBC)): %.ao: %.asm
	@echo --- $(AS) -o $@
	@$(AS) $(ASFLAGS) -o $@ $<

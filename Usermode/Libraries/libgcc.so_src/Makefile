#
# libgcc
#

-include ../Makefile.cfg

OBJS = libgcc.o
BIN = ../libgcc.so

CFLAGS  += -Wall -Werror
LDFLAGS += -soname libgcc.so

.PHONY: all clean

all: $(BIN)

clean:
	$(RM) $(BIN) $(OBJS) libgcc.so.dsm

$(BIN): $(OBJS)
	@echo -- ld -o $@
	@$(LD) $(LDFLAGS) -o $(BIN) $(OBJS)
	@$(OBJDUMP) -d $(BIN) > libgcc.so.dsm
	cp $(BIN) $(DISTROOT)/Libs

$(OBJS): %.o: %.c
	@echo -- gcc -o $@
	@$(CC) $(CFLAGS) -o $@ -c $<

#
# libgcc
#

-include ../Makefile.cfg

OBJS = libgcc.o
BIN = ../libgcc.so

CFLAGS  += -Wall -Werror
LDFLAGS += -soname libgcc.so

.PHONY: all clean install

all: $(BIN)

clean:
	$(RM) $(BIN) $(OBJS) libgcc.so.dsm

install: $(BIN)
	$(xCP) $(BIN) $(DISTROOT)/Libs

$(BIN): $(OBJS)
	@echo -- ld -o $@
	@$(LD) $(LDFLAGS) -o $(BIN) $(OBJS)
	@$(OBJDUMP) -d $(BIN) > libgcc.so.dsm

$(OBJS): %.o: %.c
	@echo -- gcc -o $@
	@$(CC) $(CFLAGS) -o $@ -c $<

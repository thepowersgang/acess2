# Acess2 Module/Driver Templater Makefile
# Makefile.tpl

-include ../../Makefile.cfg

CPPFLAGS = -I../include -I../arch/$(ARCHDIR)/include -DARCH=$(ARCH) -DBUILD_MODULE
CFLAGS = -Wall -Werror $(CPPFLAGS)

.PHONY: all clean

all: bochsvbe.kmd

%.kmd: %.o
	$(CC) -shared -nostdlib -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

#ata_x86.kmd: ata_x86.o
#bochsvbe.kmd: bochsvbe.o

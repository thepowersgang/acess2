# Acess2 Module/Driver Templater Makefile
# Makefile.tpl
ARCH = i386

CC = gcc
LD = ld

CPPFLAGS = -I../include -I../arch/$(ARCH)/include -DARCH=$(ARCH) -DBUILD_MODULE
CFLAGS = -Wall -Werror $(CPPFLAGS)

.PHONY: all clean

all: ata_x86.kmd

%.kmd:
	$(CC) -shared -nostdlib -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

ata_x86.kmd: ata_x86.o
bochsvbe.kmd: bochsvbe.o


ARCHIVE=unused
-include ../config.mk

CC = $(HOST)-gcc

all: source/libspiderscript.so
	
clean:
	CC=$(CC) make -C source/src clean

source/:
	git submodule update source/
source/libspiderscript.so: source/
	CC=$(CC) make -C source/src

install: source/libspiderscript.so
	cp source/libspiderscript.so $(EPREFIX)/lib
	cp source/spiderscript.h $(PREFIX)/include




.PHONY: all clean nativelib DiskTool NetTest

all: DiskTool NetTest

img2sif: img2sif.c
	$(CC) -o $@ $< `sdl-config --libs --cflags` -lSDL_image -Wall

nativelib:
	$(MAKE) -C $@

DiskTool NetTest: nativelib
	$(MAKE) -C $@

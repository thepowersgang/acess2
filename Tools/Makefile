
.PHONY: all clean
.PHONY: nativelib DiskTool NetTest
#.PHONY: nativelib-clean DiskTool-clean NetTest-clean

all clean: DiskTool NetTest

img2sif: img2sif.c
	$(CC) -g -std=c99 -o $@ $< `sdl-config --libs --cflags` -lSDL_image -Wall

nativelib:
	$(MAKE) -C $@ $(MAKECMDGOALS)

DiskTool NetTest: nativelib
	$(MAKE) -C $@ $(MAKECMDGOALS)

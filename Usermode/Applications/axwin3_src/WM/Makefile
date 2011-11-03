# Project: Acess2 GUI v3 Window Manager

-include ../../Makefile.cfg

# Some evil lazy evaulation, hopefully it works
CPPFLAGS += -I include/

DIR := Apps/AxWin/3.0
BIN := AxWinWM
OBJ := main.o wm.o input.o video.o ipc.o
OBJ += messageio.o
OBJ += renderer_classes.o renderer_passthru.o renderer_widget.o

LDFLAGS += -limage_sif -luri -lnet

-include ../../Makefile.tpl

#all: resources/LogoSmall.sif.res.h

%.res.h: % Makefile
	echo "#define RESOURCE_$(notdir $<) \\"| sed -e 's/\./_/g' > $@
	base64 $< | sed -e 's/.*/"&"\\/' >> $@
	echo "" >> $@

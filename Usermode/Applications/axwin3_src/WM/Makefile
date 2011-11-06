# Project: Acess2 GUI v3 Window Manager

-include ../../Makefile.cfg

CPPFLAGS += -I include/ -I ../include/

DIR := Apps/AxWin/3.0
BIN := AxWinWM
OBJ := main.o input.o video.o ipc.o
OBJ += wm.o wm_render.o
OBJ += renderer_classes.o renderer_passthru.o renderer_background.o
OBJ += renderer_widget.o renderer_widget_decorator.o

LDFLAGS += -limage_sif -luri -lnet

-include ../../Makefile.tpl

#all: resources/LogoSmall.sif.res.h

%.res.h: % Makefile
	echo "#define RESOURCE_$(notdir $<) \\"| sed -e 's/\./_/g' > $@
	base64 $< | sed -e 's/.*/"&"\\/' >> $@
	echo "" >> $@

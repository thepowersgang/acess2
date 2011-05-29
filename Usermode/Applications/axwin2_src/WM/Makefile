# Project: Acess GUI Window Manager

-include ../../Makefile.cfg

CPPFLAGS += -I../include

DIR := Apps/AxWin/1.0
BIN := AxWinWM
OBJ := main.o helpers.o commandline.o video.o input.o video_text.o
OBJ += messages.o interface.o wm.o decorator.o
OBJ += image.o

LDFLAGS += -limage_sif -luri -lnet

-include ../../Makefile.tpl

all: resources/LogoSmall.sif.res.h

%.res.h: % Makefile
	echo "#define RESOURCE_$(notdir $<) \\"| sed -e 's/\./_/g' > $@
	base64 $< | sed -e 's/.*/"&"\\/' >> $@
	echo "" >> $@

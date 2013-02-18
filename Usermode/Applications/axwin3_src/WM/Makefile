# Project: Acess2 GUI v3 Window Manager

-include ../../Makefile.cfg

CPPFLAGS += -I include/ -I ../include/
CFLAGS += -std=gnu99

DIR := Apps/AxWin/3.0
BIN := AxWinWM
OBJ := main.o input.o video.o ipc.o image.o
OBJ += wm.o wm_input.o wm_render.o wm_render_text.o wm_hotkeys.o
OBJ += decorator.o
OBJ += renderers/framebuffer.o
OBJ += renderers/background.o
OBJ += renderers/menu.o
OBJ += renderers/richtext.o
# TODO: Move to a lower makefile
OBJ += renderers/widget.o
OBJ += renderers/widget/button.o
OBJ += renderers/widget/image.o
OBJ += renderers/widget/disptext.o
OBJ += renderers/widget/textinput.o
OBJ += renderers/widget/spacer.o
OBJ += renderers/widget/subwin.o

PNGIMAGES := toolbar_new.png

LDFLAGS += -limage_sif -luri -lnet -lunicode

IMG2SIF = ../../../../Tools/img2sif

-include ../../Makefile.tpl

.PHONY: install_resources

install: install_resources

#all: resources/LogoSmall.sif.res.h

%.res.h: % Makefile
	echo "#define RESOURCE_$(notdir $<) \\"| sed -e 's/\./_/g' > $@
	base64 $< | sed -e 's/.*/"&"\\/' >> $@
	echo "" >> $@

$(IMG2SIF):
	$(MAKE) -C $(dir $(IMG2SIF)) img2sif

install_%.sif: resources/%.png
	@echo img2sif resources/$*.png
	@$(IMG2SIF) resources/$*.png resources/.$*.sif
	@$(xCP) resources/.$*.sif $(DISTROOT)/$(DIR)/$*.sif

install_resources: $(IMG2SIF) $(addprefix install_,$(PNGIMAGES:%.png=%.sif))

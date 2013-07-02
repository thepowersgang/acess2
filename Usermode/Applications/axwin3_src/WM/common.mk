
CPPFLAGS += -I include/ -I ../include/
CFLAGS += -std=gnu99

OBJ += image.o wm.o wm_input.o wm_render.o wm_render_text.o wm_hotkeys.o
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

LDFLAGS += -limage_sif -luri -lunicode

PNGIMAGES := toolbar_new.png toolbar_save.png toolbar_open.png
IMG2SIF = ../../../../Tools/img2sif

all: $(addprefix resources/,$(PNGIMAGES:%.png=.%.sif))

%.res.h: % Makefile
	echo "#define RESOURCE_$(notdir $<) \\"| sed -e 's/\./_/g' > $@
	base64 $< | sed -e 's/.*/"&"\\/' >> $@
	echo "" >> $@

$(IMG2SIF):
	$(MAKE) -C $(dir $(IMG2SIF)) img2sif

resources/.%.sif: $(IMG2SIF) Makefile resources/%.png
	@echo img2sif resources/$*.png
	@$(IMG2SIF) --rle1x32 resources/$*.png resources/.$*.sif


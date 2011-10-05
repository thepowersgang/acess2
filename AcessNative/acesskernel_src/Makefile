# AcessNative Server
# Makefile

ifeq ($(PLATFORM),)
	PLATFORM := lin
endif

KERNEL_SRC = ../../Kernel/

KERNEL_OBJ := logging.o adt.o lib.o drvutil.o debug.o
KERNEL_OBJ += vfs/main.o vfs/open.o vfs/acls.o vfs/io.o vfs/dir.o
KERNEL_OBJ += vfs/nodecache.o vfs/mount.o vfs/memfile.o vfs/select.o
KERNEL_OBJ += vfs/fs/root.o vfs/fs/devfs.o
KERNEL_OBJ += drv/vterm.o drv/fifo.o drv/proc.o

N_OBJ := main.o

OBJ := helpers.o threads.o server.o syscalls.o
OBJ += video.o keyboard.o mouse.o nativefs.o vfs_handle.o ui_sdl.o
OBJ := $(addprefix obj-$(PLATFORM)/,$(OBJ))
N_OBJ := $(addprefix obj-$(PLATFORM)/,$(N_OBJ))
K_OBJ := $(addprefix $(KERNEL_SRC)obj-$(PLATFORM)/,$(KERNEL_OBJ))

DEPFILES  = $(filter %.o,$(OBJ) $(N_OBJ) $(K_OBJ))
DEPFILES := $(DEPFILES:%=%.dep)

CPPFLAGS += -I include/ -I $(KERNEL_SRC)include/
CFLAGS += -Wall -g
LDFLAGS += -lSDL -lSDLmain -g

ifeq ($(PLATFORM),win)
	BIN := ../AcessKernel.exe
endif
ifeq ($(PLATFORM),lin)
	BIN := ../AcessKernel
	CFLAGS +=
endif

.PHONY: all clean

all: $(BIN)

clean:
	$(RM) $(BIN) $(OBJ) $(N_OBJ) $(K_OBJ) $(DEPFILES)

$(BIN): $(OBJ) $(N_OBJ) $(K_OBJ)
	@echo [LINK] -o $@
	@$(CC) $(LDFLAGS) -o $@ $(N_OBJ) $(K_OBJ) $(OBJ)

$(OBJ): obj-$(PLATFORM)/%.o: %.c
	@mkdir -p $(dir $@)
	@echo [CC] -o $@
	@$(CC) -c $< -o $@ $(CFLAGS) $(CPPFLAGS)
	@$(CC) -M $(CPPFLAGS) -MT $@ -o $@.dep $<

$(K_OBJ): $(KERNEL_SRC)obj-$(PLATFORM)/%.o: $(KERNEL_SRC)%.c
	@mkdir -p $(dir $@)
	@echo [CC] -o $@
	@$(CC) -c $< -o $@ $(CFLAGS) $(CPPFLAGS)
	@$(CC) -M $(CPPFLAGS) -MT $@ -o $@.dep $<


$(N_OBJ): obj-$(PLATFORM)/%.o: %.c
	@mkdir -p $(dir $@)
	@echo [CC] -o $@
	@$(CC) -c $< -o $@ $(CFLAGS)
	@$(CC) -M -MT $@ -o $@.dep $<

-include $(DEPFILES)

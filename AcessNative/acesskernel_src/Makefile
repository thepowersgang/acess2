# AcessNative Server
# Makefile

ifeq ($(PLATFORM),)
	PLATFORM := lin
endif

KERNEL_SRC = ../../Kernel/

KERNEL_OBJ := logging.o adt.o lib.o
KERNEL_OBJ += vfs/main.o vfs/open.o vfs/acls.o vfs/io.o vfs/dir.o vfs/nodecache.o vfs/mount.o vfs/memfile.o
KERNEL_OBJ += vfs/fs/root.o vfs/fs/devfs.o
KERNEL_OBJ += drv/vterm.o drv/fifo.o drv/proc.o

OBJ := main.o threads.o video.o keyboard.o mouse.o nativefs.o vfs_handle.o
OBJ += $(addprefix $(KERNEL_SRC),$(KERNEL_OBJ))

OBJ := $(addsuffix .$(PLATFORM),$(OBJ))

CPPFLAGS += -I include/ -I $(KERNEL_SRC)include/
CFLAGS += -Wall
LDFLAGS += -lSDL -lSDLmain

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
	$(RM) $(BIN) $(OBJ)

$(BIN): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $(OBJ)

%.o.$(PLATFORM): %.c
	$(CC) -c $< -o $@ $(CFLAGS) $(CPPFLAGS)

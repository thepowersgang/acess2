
TARGET := $(shell gcc -v 2>&1 | grep Targ | awk '{print $$2}')

KERNEL_SRC = ../../../KernelLand/Kernel/

BIN = ../DiskTool
# Kernel Sources (compiled with -ffreestanding)
K_OBJ  = vfs/main.o vfs/open.o vfs/acls.o vfs/io.o vfs/dir.o
K_OBJ += vfs/nodecache.o vfs/mount.o vfs/memfile.o vfs/select.o
K_OBJ += vfs/fs/root.o vfs/fs/devfs.o
K_OBJ += drv/proc.o
# Local kernel soruces (same as above, but located in same directory as Makefile)
L_OBJ = vfs_handles.o threads.o nativefs.o
# Native Sources (compiled as usual)
N_OBJ = main.o script.o logging.o

# Compilation Options
CFLAGS := -Wall
CPPFLAGS := -I include/
K_CPPFLAGS := -I $(KERNEL_SRC)include

# == Start of Magic ==
OBJ_PREFIX = obj/$(TARGET)/
K_OBJ_PREFIX = $(OBJ_PREFIX)_Kernel/
K_OBJ := $(addprefix $(K_OBJ_PREFIX),$(K_OBJ))
L_OBJ := $(addprefix $(OBJ_PREFIX),$(L_OBJ))
N_OBJ := $(addprefix $(OBJ_PREFIX),$(N_OBJ))

OBJ := $(N_OBJ) $(L_OBJ) $(K_OBJ)

DEPFILES  = $(filter %.o,$(OBJ))
DEPFILES := $(DEPFILES:%=%.dep)


.PHONY: all clean

all: $(BIN)

clean:
	$(RM) -f $(OBJ) $(DEPFILES) $(BIN)

$(BIN): $(OBJ)
	$(CC) -o $(BIN) $(OBJ) $(LDFLAGS)

$(K_OBJ): $(K_OBJ_PREFIX)%.o: $(KERNEL_SRC)%.c
	@mkdir -p $(dir $@)
	$(CC) -c $< -o $@ -ffreestanding $(CFLAGS) $(CPPFLAGS) $(K_CPPFLAGS) -MMD -MP -MF $@.dep

$(L_OBJ): $(OBJ_PREFIX)%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) -c $< -o $@ -ffreestanding $(CFLAGS) $(CPPFLAGS) $(K_CPPFLAGS) -MMD -MP -MF $@.dep

$(N_OBJ): $(OBJ_PREFIX)%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) -c $< -o $@ $(CFLAGS) $(CPPFLAGS) -MMD -MP -MF $@.dep

-include $(DEPFILES)


KERNEL_DIR := ../../KernelLand/Kernel

NOBJ := logging.o misc.o threads_int.o

LOBJ := threads.o time.o mutex.o rwlock.o semaphore.o

KOBJ += vfs/main.o vfs/open.o vfs/acls.o vfs/io.o vfs/dir.o
KOBJ += vfs/nodecache.o vfs/mount.o vfs/memfile.o # vfs/select.o
KOBJ += vfs/fs/root.o vfs/fs/devfs.o
KOBJ += drv/proc.o

NOBJ := $(NOBJ:%.o=obj/%.o)
LOBJ := $(LOBJ:%.o=obj/%.o)
KOBJ := $(KOBJ:%.o=obj/_Kernel/%.o)
OBJ := $(NOBJ) $(LOBJ) $(KOBJ)
BIN := ../libnativelib.a

CFLAGS := -Wall -std=c99 -Werror
CPPFLAGS := -I include


.PHONY: all clean

all: $(BIN)

clean:
	$(RM) $(BIN) $(OBJ)

$(BIN): $(OBJ)
	ar cru $(BIN) $(OBJ)

$(NOBJ): obj/%.o: %.c
	@echo [CC Native] $@
	@mkdir -p $(dir $@)
	@$(CC) -o $@ -c $< $(CFLAGS) $(CPPFLAGS)
$(LOBJ): obj/%.o: %.c
	@echo [CC Local] $@
	@mkdir -p $(dir $@)
	@$(CC) -o $@ -c $< $(CFLAGS) $(CPPFLAGS) -I $(KERNEL_DIR)/include

$(KOBJ): obj/_Kernel/%.o: $(KERNEL_DIR)/%.c
	@echo [CC Kernel] $@
	@mkdir -p $(dir $@)
	@$(CC) -o $@ -c $< $(CFLAGS) $(CPPFLAGS) -I $(KERNEL_DIR)/include


KERNEL_DIR := ../../KernelLand/Kernel

NOBJ := logging.o misc.o threads_int.o

LOBJ := threads.o time.o dummy_iocache.o
# mutex.o rwlock.o semaphore.o

KOBJ += vfs/main.o vfs/open.o vfs/acls.o vfs/io.o vfs/dir.o
KOBJ += vfs/nodecache.o vfs/mount.o vfs/memfile.o # vfs/select.o
KOBJ += vfs/fs/root.o vfs/fs/devfs.o
KOBJ += drv/proc.o
KOBJ += mutex.o rwlock.o semaphore.o
KOBJ += utf16.o

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
	ar cr $(BIN) $(OBJ)

$(NOBJ): obj/%.o: %.c Makefile
	@echo [CC Native] $@
	@mkdir -p $(dir $@)
	@$(CC) -o $@ -c $< -MMD -MF $@.dep -MT $@ -MP $(CFLAGS) $(CPPFLAGS)
$(LOBJ): obj/%.o: %.c Makefile
	@echo [CC Local] $@
	@mkdir -p $(dir $@)
	@$(CC) -o $@ -c $< -MMD -MF $@.dep -MT $@ -MP $(CFLAGS) $(CPPFLAGS) -I $(KERNEL_DIR)/include

$(KOBJ): obj/_Kernel/%.o: $(KERNEL_DIR)/%.c Makefile
	@echo [CC Kernel] $@
	@mkdir -p $(dir $@)
	@$(CC) -o $@ -c $< -MMD -MF $@.dep -MT $@ -MP $(CFLAGS) $(CPPFLAGS) -I $(KERNEL_DIR)/include

-include $(OBJ:%=%.dep)

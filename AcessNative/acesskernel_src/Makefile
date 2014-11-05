# AcessNative Server
# Makefile

-include ../../Makefile.Version.cfg
-include Makefile.BuildNum
ifeq ($(BUILD_NUM),)
BUILD_NUM = 1
endif

ifeq ($(PLATFORM),)
	PLATFORM := lin
endif

KERNEL_SRC = ../../KernelLand/Kernel/
LDACESS_SRC = ../../Usermode/Libraries/ld-acess.so_src/

# - Kernel objects (from KernelLand/Kernel)
KERNEL_OBJ := logging.o adt.o lib.o debug.o messages.o drvutil_disk.o drvutil_video.o memfs_helpers.o
KERNEL_OBJ += mutex.o semaphore.o rwlock.o workqueue.o events.o
#KERNEL_OBJ += libc.o
KERNEL_OBJ += vfs/main.o vfs/open.o vfs/acls.o vfs/io.o vfs/dir.o
KERNEL_OBJ += vfs/nodecache.o vfs/mount.o vfs/memfile.o vfs/select.o
KERNEL_OBJ += vfs/fs/root.o vfs/fs/devfs.o
KERNEL_OBJ += drv/fifo.o drv/proc.o drv/dgram_pipe.o	# drv/shm.o
KERNEL_OBJ += drv/vterm.o drv/vterm_font.o drv/vterm_output.o drv/vterm_input.o drv/vterm_termbuf.o
KERNEL_OBJ += drv/vterm_vt100.o drv/vterm_2d.o
KERNEL_OBJ += drv/pty.o
# - Native (unsullied) objects
N_OBJ := main.o net_wrap.o
# - Local objects (use the kernel includes)
OBJ := helpers.o threads.o threads_glue.o server.o syscalls.o time.o
OBJ += video.o keyboard.o mouse.o nativefs.o vfs_handle.o ui_sdl.o
OBJ += net.o syscall_getpath.o

BUILDINFO_OBJ := obj-$(PLATFORM)/buildinfo.o
BUILDINFO_SRC := $(BUILDINFO_OBJ:%.o=%.c)

OBJ := $(addprefix obj-$(PLATFORM)/,$(OBJ))
N_OBJ := $(addprefix obj-$(PLATFORM)/,$(N_OBJ))
K_OBJ := $(addprefix $(KERNEL_SRC)obj-native-$(PLATFORM)/,$(KERNEL_OBJ))

DEPFILES  = $(filter %.o,$(OBJ) $(N_OBJ) $(K_OBJ))
DEPFILES := $(DEPFILES:%=%.dep)

KCPPFLAGS = -I include/ -I $(KERNEL_SRC)include/ -I$(LDACESS_SRC)include_exp/ -D KERNEL_VERSION=${ACESS_VERSION}
CFLAGS += -Wall -g -std=gnu99
CPPFLAGS += $(shell sdl-config --cflags) -I /usr/include/
LDFLAGS += $(shell sdl-config --libs) -g -Wl,--defsym,__buildnum=$(BUILD_NUM)
LDFLAGS += -Wl,-Map,obj-$(PLATFORM)/Map.txt

ifeq ($(PLATFORM),win)
	BIN := ../AcessKernel.exe
	LDFLAGS += -lws2_32 -lpthread
endif
ifeq ($(PLATFORM),lin)
	BIN := ../AcessKernel
	CFLAGS += 
	LDFLAGS += -lpthread
endif

.PHONY: all clean

all: $(BIN)

clean:
	$(RM) $(BIN) $(OBJ) $(N_OBJ) $(K_OBJ) $(DEPFILES)

$(BIN): $(OBJ) $(N_OBJ) $(K_OBJ) $(BUILDINFO_OBJ)
	@echo [LINK] -o $@
	@$(CC) -o $@ $(N_OBJ) $(LDFLAGS) $(K_OBJ) $(OBJ) $(BUILDINFO_OBJ)
	@echo BUILD_NUM = $$(( $(BUILD_NUM) + 1 )) > Makefile.BuildNum

$(OBJ): obj-$(PLATFORM)/%.o: %.c
	@mkdir -p $(dir $@)
	@echo [CC] -o $@
	@$(CC) -c $< -o $@ $(CFLAGS) $(KCPPFLAGS) $(CPPFLAGS) -MMD -MP -MF $@.dep

$(K_OBJ): $(KERNEL_SRC)obj-native-$(PLATFORM)/%.o: $(KERNEL_SRC)%.c
	@mkdir -p $(dir $@)
	@echo [CC] -o $@
	@$(CC) -ffreestanding -c $< -o $@ $(CFLAGS) $(KCPPFLAGS) $(CPPFLAGS) -MMD -MP -MF $@.dep


$(N_OBJ): obj-$(PLATFORM)/%.o: %.c
	@mkdir -p $(dir $@)
	@echo [CC] -o $@
	@$(CC) -c $< -o $@ $(CPPFLAGS) $(CFLAGS) -MMD -MP -MF $@.dep

$(BUILDINFO_SRC): $(filter-out $(BUILDINFO_OBJ), $(OBJ)) Makefile
	@echo "" > $@
	$(eval _GITHASH=$(shell (git log -n 1 | head -n 1 | awk '{print $$2}') 2>/dev/null || echo UNK))
	$(eval _GITCHANGED=$(shell (git status --porcelain | grep -c '^ M ') 2>/dev/null || echo UNK))
	$(eval _HOSTNAME=$(shell hostname --fqdn 2>/dev/null || hostname || echo UNK))
	@echo "const char gsKernelVersion[] = \"$(ACESS_VERSION)\";" >> $@
	@echo "const char gsGitHash[] = \"$(_GITHASH)\";" >> $@
	@echo "const char gsBuildInfo[] = \"AcessNative $(ACESS_VERSION) $(ARCH)-$(PLATFORM)\\\\r\\\\n\"" >> $@
	@echo "                           \"Build $(_HOSTNAME):$(BUILD_NUM) Git $(_GITHASH) - $(_GITCHANGED) modified\";" >> $@
	@echo "const int giBuildNumber = $(BUILD_NUM);" >> $@
$(BUILDINFO_OBJ): $(BUILDINFO_SRC)
	@echo [CC] -o $@
	@$(CC) -o $@ -c $< $(CFLAGS) $(CPPFLAGS)

-include $(DEPFILES)

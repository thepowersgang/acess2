
TARGET := $(shell gcc -v 2>&1 | grep Targ | awk '{print $$2}')

include ../../Makefile.Version.cfg
-include Makefile.BuildNum
ifeq ($(BUILD_NUM),)
BUILD_NUM = 1
endif


KERNEL_SRC = ../../KernelLand/Kernel/
MODULE_SRC = ../../KernelLand/Modules/

BIN = ../disktool
# Kernel Sources (compiled with -ffreestanding)
K_OBJ := lib.o
K_OBJ += drvutil_disk.o drv/proc.o
# Modules
MODULES := Storage/LVM Filesystems/FAT Filesystems/Ext2 Filesystems/NTFS
# Local kernel soruces (same as above, but located in same directory as Makefile)
L_OBJ = vfs_handles.o nativefs.o actions.o
# Native Sources (compiled as usual)
N_OBJ = main.o script.o helpers.o

# Compilation Options
CFLAGS := -Wall -std=gnu99 -g -Werror
CPPFLAGS := -I include/ -I ../nativelib
CPPFLAGS += -I../../Usermode/Libraries/ld-acess.so_src/include_exp/	# Syscalls
K_CPPFLAGS := -I $(KERNEL_SRC)include -I $(MODULE_SRC)
LDFLAGS += -Wl,--defsym,__buildnum=$(BUILD_NUM) -g -L ../ -lnativelib

BUILDINFO_OBJ := obj/$(TARGET)/buildinfo.o
BUILDINFO_SRC := $(BUILDINFO_OBJ:%.o=%.c)

# ====================
# == Start of Magic ==
# ====================
# -- Load modules ---
$(foreach module,$(MODULES), $(eval include $(MODULE_SRC)$(module)/Makefile) $(eval M_OBJ += $(addprefix $(module)/,$(OBJ))) )

# -- Apply Prefixes to object paths
OBJ_PREFIX = obj/$(TARGET)/
K_OBJ_PREFIX = $(OBJ_PREFIX)_Kernel/
M_OBJ_PREFIX = $(OBJ_PREFIX)_Module/
K_OBJ := $(addprefix $(K_OBJ_PREFIX),$(K_OBJ))
M_OBJ := $(addprefix $(M_OBJ_PREFIX),$(M_OBJ))
L_OBJ := $(addprefix $(OBJ_PREFIX),$(L_OBJ))
N_OBJ := $(addprefix $(OBJ_PREFIX),$(N_OBJ))

OBJ := $(N_OBJ) $(L_OBJ) $(K_OBJ) $(M_OBJ) $(BUILDINFO_OBJ)

DEPFILES  = $(filter %.o,$(OBJ))
DEPFILES := $(DEPFILES:%=%.dep)


.PHONY: all clean

all: $(BIN)

clean:
	$(RM) -f $(OBJ) $(DEPFILES) $(BIN)

$(BIN): $(OBJ) ../libnativelib.a
	@echo [CC Link] -o $(BIN)
	@$(CC) -o $(BIN) $(OBJ) $(LDFLAGS)
	@echo BUILD_NUM = $$(( $(BUILD_NUM) + 1 )) > Makefile.BuildNum

$(M_OBJ): $(M_OBJ_PREFIX)%.o: $(MODULE_SRC)%.c
	@mkdir -p $(dir $@)
	@echo [CC Module] -o $@
	@$(CC) -c $< -o $@ -ffreestanding $(CFLAGS) $(CPPFLAGS) $(K_CPPFLAGS) -MMD -MP -MF $@.dep

$(K_OBJ): $(K_OBJ_PREFIX)%.o: $(KERNEL_SRC)%.c
	@mkdir -p $(dir $@)
	@echo [CC Kernel] -o $@
	@$(CC) -c $< -o $@ -ffreestanding $(CFLAGS) $(CPPFLAGS) $(K_CPPFLAGS) -MMD -MP -MF $@.dep

$(L_OBJ): $(OBJ_PREFIX)%.o: %.c
	@mkdir -p $(dir $@)
	@echo [CC Local] -o $@
	@$(CC) -c $< -o $@ -ffreestanding $(CFLAGS) $(CPPFLAGS) $(K_CPPFLAGS) -MMD -MP -MF $@.dep

$(N_OBJ): $(OBJ_PREFIX)%.o: %.c
	@mkdir -p $(dir $@)
	@echo [CC Native] -o $@
	@$(CC) -c $< -o $@ $(CFLAGS) $(CPPFLAGS) -MMD -MP -MF $@.dep

# Hacky buildinfo.c file
$(BUILDINFO_SRC): $(filter-out $(BUILDINFO_OBJ), $(OBJ)) Makefile
	@echo "" > $@
	@echo "const char gsKernelVersion[] = \"$(ACESS_VERSION)\";" >> $@
	@echo "const char gsGitHash[] = \""`git log -n 1 | head -n 1 | awk '{print $$2}'`"\";" >> $@
	@echo "const int giBuildNumber = $(BUILD_NUM);" >> $@
	@echo "const char gsBuildInfo[] = \"Acess2 DiskTool v$(ACESS_VERSION)\";" >> $@
$(BUILDINFO_OBJ): $(BUILDINFO_SRC)
	@echo [CC] -o $@
	@$(CC) -o $@ -c $< $(CFLAGS) $(CPPFLAGS)

$(OBJ): Makefile

-include $(DEPFILES)

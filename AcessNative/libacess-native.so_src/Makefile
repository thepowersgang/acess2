#
#

ifeq ($(PLATFORM),)
	PLATFORM := lin
endif

OBJ := main.o syscalls.o request.o memory.o exports.o heap.o
OBJ := $(addprefix obj-$(PLATFORM)/,$(OBJ))

ifeq ($(PLATFORM),win)
	BIN := ../libacess-native.dll
	LDFLAGS += -lws2_32
endif
ifeq ($(PLATFORM),lin)
	BIN := ../libacess-native.so
endif

CFLAGS   += -Wall
CFLAGS   += -Werror
CFLAGS   += -g -shared -fPIC
CPPFLAGS += -DARCHDIR_is_x86_64=1
LDFLAGS  += -g -shared -Wl,--no-undefined -lc

DEPFILES  = $(filter %.o,$(OBJ))
DEPFILES := $(DEPFILES:%=%.dep)

.PHONY: all clean

all: $(BIN)

clean:
	$(RM) $(BIN) $(OBJ) $(DEPFILES)

$(BIN): $(OBJ)
	$(CC) -o $@ $(OBJ) $(LDFLAGS)
	objdump -S $@ > $@.dsm

obj-$(PLATFORM)/%.o: %.c
	@mkdir -p $(dir $@)
	@echo [CC] -o $@
	@$(CC) -c $< -o $@ $(CFLAGS) $(CPPFLAGS)
	@$(CC) -M $(CPPFLAGS) -MT $@ -o $@.dep $<

-include $(DEPFILES)


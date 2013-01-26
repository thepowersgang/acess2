
KERNEL_DIR := ../../KernelLand/Kernel

NOBJ := logging.o misc.o
KOBJ := threads.o time.o mutex.o

NOBJ := $(NOBJ:%.o=obj/%.o)
KOBJ := $(KOBJ:%.o=obj/%.o)
OBJ := $(NOBJ) $(KOBJ)
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
$(KOBJ): obj/%.o: %.c
	@echo [CC Kernel] $@
	@mkdir -p $(dir $@)
	@$(CC) -o $@ -c $< $(CFLAGS) $(CPPFLAGS) -I $(KERNEL_DIR)/include


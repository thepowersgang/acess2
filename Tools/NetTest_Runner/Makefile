
OBJ := main.o net.o stack.o
OBJ += tcp.o
OBJ += ip.o arp.o
OBJ += link.o
OBJ += test_arp.o test_tcp.o
BIN := ../nettest_runner

CFLAGS := -Wall -std=c99
CPPFLAGS := -Iinclude
LIBS := 

OBJ := $(OBJ:%=obj/%)

.PHONY: all clean

all: $(BIN)

clean:
	$(RM) $(OBJ) $(BIN)

$(BIN): $(OBJ)
	@echo [CC] -o $@
	@$(CC) $(LINKFLAGS) -o $@ $(OBJ) $(LIBS)

obj/%.o: %.c
	@mkdir -p $(dir $@)
	@echo [CC] -c -o $@
	@$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ -c $< -MMD -MP

-include $(OBJ:%.o=%.d)


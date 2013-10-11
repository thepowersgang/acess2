
OBJS = main.o inifile.o build.o udiprops.o
BIN = ../udibuild
CFLAGS = -std=c99 -MMD -MP -g

OBJS := $(OBJS:%=obj/%)

.PHONY: all clean

all: $(BIN)

clean:
	$(RM) $(BIN) $(OBJS) $(OBJS:%.o=%.d)

$(BIN): $(OBJS)
	@echo --- [CC] $@
	@$(CC) -o $(BIN) $(OBJS) $(LDFLAGS)

obj/%.o: %.c Makefile
	@mkdir -p $(dir $@)
	@echo --- [CC] $@
	@$(CC) -o $@ -c $< $(CFLAGS)

-include $(OBJS:%.o=%.d)


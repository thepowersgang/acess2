

include $(dir $(lastword $(MAKEFILE_LIST)))../../Makefile.cfg

COMPONENTS = utilities tables
BIN := ../$(ARCH)/libacpica.a

ACPICAROOT := acpica-unix-20121114/
COMPDIR := $(ACPICAROOT)source/components/

CPPFLAGS += -I $(ACPICAROOT)source/include

SRCS := $(foreach comp,$(COMPONENTS),$(wildcard $(COMPDIR)$(comp)/*.c))
OBJS := $(SRCS:$(COMPDIR)%.c=obj-$(ARCH)/%.o)

.PHONY: all clean

$(BIN): $(OBJS)
	ar -cu $@ $(OBJS)

obj-$(ARCH)/%.o: $(COMPDIR)%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ -c $< 
	@$(CC) -M -MP -MT $@ $(CPPFLAGS) $< -o $@.dep


include $(BASE)header.mk

# Rules
ASFLAGS-$(DIR)  := -felf -D ARCHDIR=$(ARCHDIR) -D __ASSEMBLER__=1
CPPFLAGS-$(DIR) := -I$(ACESSDIR)/Usermode/include/ -DARCHDIR=$(ARCHDIR) -DARCHDIR_is_$(ARCHDIR)=1
CFLAGS-$(DIR)   := -g -Wall -fno-stack-protector -O3
LDFLAGS-$(DIR)  := -T $(OUTPUTDIR)Libs/acess.ld -rpath-link $(OUTPUTDIR)Libs -L $(OUTPUTDIR)Libs -I /Acess/Libs/ld-acess.so -lld-acess -lc $(OUTPUTDIR)Libs/crtbegin.o $(OUTPUTDIR)Libs/crtend.o

SUB_DIRS = $(wildcard $(DIR)*/rules.mk)

.PHONY: all-$(DIR) clean-$(DIR)
all-$(DIR): $(addprefix all-,$(dir $(SUB_DIRS)))
clean-$(DIR): $(addprefix clean-,$(dir $(SUB_DIRS)))

include $(SUB_DIRS)

include $(BASE)footer.mk


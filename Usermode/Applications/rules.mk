include $(BASE)header.mk

# Rules
ASFLAGS-$(DIR)  := -felf -D ARCHDIR=$(ARCHDIR) -D __ASSEMBLER__=1
CPPFLAGS-$(DIR) := -ffreestanding -I$(ACESSDIR)/Usermode/include/ -DARCHDIR=$(ARCHDIR) -DARCHDIR_is_$(ARCHDIR)=1
CPPFLAGS-$(DIR) += $(addprefix -I,$(wildcard $(ACESSUSERDIR)Libraries/*/include_exp/))
CFLAGS-$(DIR)   := -g -Wall -fno-stack-protector -O3
LDFLAGS-$(DIR)  := -T $(OUTPUTDIR)Libs/acess.ld -rpath-link $(OUTPUTDIR)Libs -L $(OUTPUTDIR)Libs -I /Acess/Libs/ld-acess.so -lld-acess -lc $(OUTPUTDIR)Libs/crtbegin.o $(OUTPUTDIR)Libs/crtend.o

SUB_DIRS = $(wildcard $(DIR)*/rules.mk)
EXTRA_DEP-$(DIR) += $(OUTPUTDIR)Libs/acess.ld $(OUTPUTDIR)Libs/crtbegin.o $(OUTPUTDIR)Libs/crt0.o $(OUTPUTDIR)Libs/crtend.o

.PHONY: all-$(DIR) clean-$(DIR)
all-$(DIR): $(addprefix all-,$(dir $(SUB_DIRS)))
clean-$(DIR): $(addprefix clean-,$(dir $(SUB_DIRS)))

include $(SUB_DIRS)

include $(BASE)footer.mk


include $(BASE)header.mk

# Rules
ASFLAGS-$(DIR)  := -D ARCHDIR=$(ARCHDIR) -D __ASSEMBLER__=1
CPPFLAGS-$(DIR) := -I$(ACESSDIR)/Usermode/include/ -DARCHDIR=$(ARCHDIR) -DARCHDIR_is_$(ARCHDIR)=1
CFLAGS-$(DIR)   := -g -Wall -fPIC -fno-stack-protector -O3
LDFLAGS-$(DIR)  := -g -nostdlib -shared -I/Acess/Libs/ld-acess.so -lld-acess -e SoMain -x -L$(OUTPUTDIR)Libs/ --no-undefined

SUB_DIRS = $(wildcard $(DIR)*/rules.mk)

.PHONY: all-$(DIR) clean-$(DIR)
all-$(DIR): open-$(DIR) $(addprefix all-,$(dir $(SUB_DIRS))) close-$(DIR) 
clean-$(DIR): $(addprefix clean-,$(dir $(SUB_DIRS)))

include $(SUB_DIRS)

include $(BASE)footer.mk

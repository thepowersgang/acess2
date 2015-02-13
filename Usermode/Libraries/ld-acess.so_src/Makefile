# Acess Dynamic Linker (ELF) Version 1
#  LD-ACESS.SO
#  Makefile

-include ../Makefile.cfg

OBJ := main.o lib.o loadlib.o export.o elf.o pe.o
OBJ += arch/$(ARCHDIR).ao_
BIN = ld-acess.so
EXTRABIN := libld-acess.so
EXTRACLEAN = $(_OBJPREFIX)_stublib.o
INCFILES := sys/sys.h

#CPPFLAGS += -D DISABLE_ELF64
CFLAGS   = -g -Wall -fno-builtin -fno-stack-protector -fPIC -std=c99 -ffreestanding
# -fno-leading-underscore
CFLAGS  += $(CPPFLAGS)
LDFLAGS  = -ffreestanding -nostdlib -g -Wl,-T,arch/$(ARCHDIR).ld -Map map.txt --export-dynamic -x
LIBS := $(LIBGCC_PATH)

ifeq ($(ARCH),native)
XBIN := $(addprefix $(OUTPUTDIR)Libs/,$(EXTRABIN)) 
$(XBIN): obj-$(ARCH)/_stublib.o
all: $(XBIN)
CFLAGS += -ffreestanding
LDFLAGS := 
BIN := 
OBJ :=
endif

include ../Makefile.tpl

# create libld-acess.so
$(_XBIN): $(_OBJPREFIX)_stublib.o
	@echo [LD] -o -shared libld-acess.so
	$V$(LD) -shared -o $@ $<
#	@$(LD) $(LDFLAGS) -o $@ $(OBJ)


# Override .ao to look in the object prefix for the source
$(_OBJPREFIX)arch/$(ARCHDIR).ao_: $(_OBJPREFIX)arch/$(ARCHDIR).$(ASSUFFIX) Makefile
	@echo [AS] -o $@
	@mkdir -p $(dir $@)
	#$V$(AS) $(ASFLAGS) -o $@ $<
	$V$(CC) -c $(ASFLAGS) -o $@ $<

#.PRECIOUS: $(OBJ:%.ao=%.asm)

# Preprocessing objects if needed
$(_OBJPREFIX)arch/$(ARCHDIR).$(ASSUFFIX): arch/$(ARCHDIR).$(ASSUFFIX).h arch/syscalls.s.h
	@echo [CPP] -o $@
	@mkdir -p $(dir $@)
	$V$(CPP) $(CPPFLAGS) -P -D__ASSEMBLER__ $< -o $@

$(_OBJPREFIX)arch/$(ARCHDIR).$(ASSUFFIX): $(ACESSDIR)/KernelLand/Kernel/include/syscalls.h

$(_OBJPREFIX)_stublib.o: arch/syscalls.s.h


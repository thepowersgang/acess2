#
# Acess2 Global Makefile
#

# Config Options
SOURCE_DIR = 
OBJECT_DIR = obj-$(ARCH)/
OBJECT_SUFFIX =

# Functions used later
fcn_src2obj_int = \
	$(patsubst %,%$(OBJECT_SUFFIX).o,$(filter %.c,$1)) \
	$(patsubst %,%$(OBJECT_SUFFIX).o,$(filter %.cpp,$1)) \
	$(patsubst %,%$(OBJECT_SUFFIX).o,$(filter %.cc,$1)) \
	$(patsubst %,%$(OBJECT_SUFFIX).o,$(filter %.S,$1)) \
	$(patsubst %,%$(OBJECT_SUFFIX).o,$(filter %.asm,$1))
fcn_mkobj = $(addprefix $(DIR)$(OBJECT_DIR),$(patsubst $(SOURCE_DIR)%,%,$1))
fcn_src2obj = $(call fcn_mkobj, $(call fcn_src2obj_int,$1))
fcn_obj2src = $(subst $(OBJECT_DIR),$(SOURCE_DIR),$(patsubst %$(OBJECT_SUFFIX).o,%,$1))

fcn_addbin = $(eval ALL_OBJ:=$(ALL_OBJ) $2) $(eval ALL_BIN:=$(ALL_BIN) $1) $(foreach f,$2 $1,$(eval _DIR-$f := $(DIR))) $(eval $1: $2) $(eval OBJ-$(DIR):=$(OBJ-$(DIR)) $2) $(eval BIN-$(DIR):=$(BIN-$(DIR)) $1)

# Start of Voodoo code
_REL_POS := $(dir $(lastword $(MAKEFILE_LIST)))
BASE = $(abspath $(_REL_POS))/
SUB_DIRS = $(wildcard $(BASE)*/rules.mk)
#$(warning $(SUB_DIRS))
ifeq ($(_REL_POS),./)
    # Root makefile
    DEFAULT_RULES := $(dir $(SUB_DIRS))
else
    # Build part of the tree
    DEFAULT_RULES := $(abspath $(shell pwd))/
#    $(warning $(DEFAULT_RULES))
endif

include $(BASE)Makefile.cfg

.PHONY: all clean

all: $(addprefix all-,$(DEFAULT_RULES))
clean: $(addprefix clean-,$(DEFAULT_RULES))

# Sub-directory rules
x = x
include $(SUB_DIRS)


# Transforms LDFLAGS -l arguments into library binary paths
fcn_getlibs = $(foreach f,$(patsubst -l%,lib%.so,$(filter -l%,$(LDFLAGS-$(_DIR-$1)))),$(filter %/$f,$(ALL_BIN)))

# === Rules ===
# - Binds source files to object targets
fcn_mkrule = $(eval $1: $(call fcn_obj2src,$1))
$(foreach f,$(filter %.cpp$(OBJECT_SUFFIX).o,$(ALL_OBJ)), $(call fcn_mkrule,$f))
$(foreach f,$(filter %.cc$(OBJECT_SUFFIX).o,$(ALL_OBJ)), $(call fcn_mkrule,$f))
$(foreach f,$(filter %.c$(OBJECT_SUFFIX).o,$(ALL_OBJ)), $(call fcn_mkrule,$f))
$(foreach f,$(filter %.S$(OBJECT_SUFFIX).o,$(ALL_OBJ)), $(call fcn_mkrule,$f))
$(foreach f,$(filter %.asm$(OBJECT_SUFFIX).o,$(ALL_OBJ)), $(call fcn_mkrule,$f))
# - Bind extra dependencies and libraries to objects
$(foreach f,$(ALL_BIN), $(eval $f: $(EXTRA_DEP-$(_DIR-$f)) $(call fcn_getlibs,$f)))

# --- Object Files ---
# C++ (.cpp)
%.cpp$(OBJECT_SUFFIX).o:
	$(eval _dir=$(_DIR-$@))
	$(eval <=$(call fcn_obj2src,$@))
	@echo [CXX] -o $<
	@mkdir -p $(dir $@)
	@$(CCPP) $(CXXFLAGS) $(CPPFLAGS) $(CXXFLAGS-$(_dir)) $(CPPFLAGS-$(_dir))-c $(_src) -o $@
# C++ (.cc)
%.cc$(OBJECT_SUFFIX).o:
	$(eval _dir=$(_DIR-$@))
	$(eval _src=$(call fcn_obj2src,$@))
	@echo [CXX] -o $<
	@mkdir -p $(dir $@)
	@$(CCPP) $(CXXFLAGS) $(CXXFLAGS-$(_dir)) $(CPPFLAGS-$(_dir)) -c $(_src) -o $@
# C (.c)
%.c$(OBJECT_SUFFIX).o:
	$(eval _dir=$(_DIR-$@))
	$(eval _src=$(call fcn_obj2src,$@))
	@echo [CC] -o $@
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) $(CPPFLAGS) $(CFLAGS-$(_dir)) $(CPPFLAGS-$(_dir)) -c $(_src) -o $@
# Assembly (.S)
%.S$(OBJECT_SUFFIX).o:
	$(eval _dir=$(_DIR-$@))
	$(eval _src=$(call fcn_obj2src,$@))
	@echo [AS] -o $@
	@mkdir -p $(dir $@)
	@$(AS) $(ASFLAGS) $(ASFLAGS-$(_dir)) -o $@ $(_src)
	@$(AS) $(ASFLAGS) $(ASFLAGS-$(_dir)) -o $@.dep $(_src) -M
# Assembly (.asm)
%.asm$(OBJECT_SUFFIX).o:
	$(eval _dir=$(_DIR-$@))
	$(eval _src=$(call fcn_obj2src,$@))
	@echo [AS] -o $@
	@mkdir -p $(dir $@)
	@$(AS) $(ASFLAGS) $(ASFLAGS-$(_dir)) -o $@ $(_src)
	@$(AS) $(ASFLAGS) $(ASFLAGS-$(_dir)) -o $@ $(_src) -MT $@ -MD $@.dep

# --- Binaries ---
# Static Library (.a)
%.a:
	$(eval _dir=$(_DIR-$@))
	@echo [AR] ru $@
	@$(RM) $@
	@$(AR) ru $@ $(OBJ-$(_dir))
# Dynamic Library (.so)
%.so:
	$(eval _dir=$(_DIR-$@))
	@echo [LD] -shared -o $@
	@$(LD) $(LDFLAGS) -shared -soname $(basename $@) -o $@ $(filter %.o,$^) $(LDFLAGS-$(_dir))
# Executable (.bin)
%.bin:
	$(eval _dir=$(_DIR-$@))
	@echo [LD] -o $@
	@$(LD) $(LDFLAGS) -o $@ $(OBJ-$(_dir)) $(LDFLAGS-$(_dir))
	@$(CP) $@ $(@:%.bin=%)   
$(OUTPUTDIR)%:
	$(eval _dir=$(_DIR-$@))
	@echo [LD] -o $@
	@$(LD) $(LDFLAGS) -o $@ $(OBJ-$(_dir)) $(LDFLAGS-$(_dir))

-include $(ALL_OBJ:%=%.dep)

%.asm: %.asm.o
 

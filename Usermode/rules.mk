#
# Acess2 Global Makefile
#

# Config Options
SOURCE_DIR = 
OBJECT_DIR = obj-$(ARCH)
OBJECT_SUFFIX =

# Functions used later
fcn_src2obj_int = \
	$(patsubst %,%$(OBJECT_SUFFIX).o,$(filter %.c,$1)) \
	$(patsubst %,%$(OBJECT_SUFFIX).o,$(filter %.cpp,$1)) \
	$(patsubst %,%$(OBJECT_SUFFIX).o,$(filter %.cc,$1)) \
	$(patsubst %,%$(OBJECT_SUFFIX).o,$(filter %.S,$1)) \
	$(patsubst %,%$(OBJECT_SUFFIX).o,$(filter %.asm,$1))
fcn_mkobj = $(addprefix $(DIR)$(OBJECT_DIR)/,$(patsubst $(SOURCE_DIR)%,%,$1))
fcn_src2obj = $(call fcn_mkobj, $(call fcn_src2obj_int,$1))

# Start of Voodoo code
SUB_DIRS = $(wildcard */rules.mk)

_REL_POS := $(dir $(lastword $(MAKEFILE_LIST)))
BASE = $(abspath $(_REL_POS))/
ifeq ($(_REL_POS),./)
    # Root makefile
    DEFAULT_RULES := $(dir $(SUB_DIRS))
else
    # Build part of the tree
    DEFAULT_RULES := $(abspath $(pwd)/$(_REL_POS))
endif

include $(_REL_POS)../Makefile.cfg

.PHONY: all clean

all: $(BASE)obj_rules.mk $(addprefix all-,$(DEFAULT_RULES))
clean: $(addprefix clean-,$(DEFAULT_RULES))

# Sub-directory rules
x = x
include $(SUB_DIRS)


# === Rules ===
fcn_obj2src = $(subst $(OBJECT_DIR)/,$(SOURCE_DIR)/,$(patsubst %$(OBJECT_SUFFIX).o,%,$1))

ifeq (x,)
$(foreach file,$(filter %.cpp$(OBJECT_SUFFIX).o,$(ALL_OBJ)), $(eval $f: $(call fcn_obj2src,$f)))
$(foreach file,$(filter %.cc$(OBJECT_SUFFIX).o,$(ALL_OBJ)), $(eval $f: $(call fcn_obj2src,$f)))
$(foreach file,$(filter %.c$(OBJECT_SUFFIX).o,$(ALL_OBJ)), $(eval $f: $(call fcn_obj2src,$f)))
$(foreach file,$(filter %.S$(OBJECT_SUFFIX).o,$(ALL_OBJ)), $(eval $f: $(call fcn_obj2src,$f))))
$(foreach file,$(filter %.asm$(OBJECT_SUFFIX).o,$(ALL_OBJ)), $(eval $f: $(call fcn_obj2src,$f)))
else
.PHONY: $(BASE)obj_rules.mk
$(BASE)obj_rules.mk:
	@echo "$(foreach f,$(filter %.cpp$(OBJECT_SUFFIX).o,$(ALL_OBJ)),$f: $(call fcn_obj2src,$f)\n)" > $@
	@echo "$(foreach f,$(filter %.cc$(OBJECT_SUFFIX).o,$(ALL_OBJ)),$f: $(call fcn_obj2src,$f)\n)" >> $@
	@echo "$(foreach f,$(filter %.c$(OBJECT_SUFFIX).o,$(ALL_OBJ)),$f: $(call fcn_obj2src,$f)\n)" >> $@
	@echo "$(foreach f,$(filter %.S$(OBJECT_SUFFIX).o,$(ALL_OBJ)),$f: $(call fcn_obj2src,$f)\n)" >> $@
	@echo "$(foreach f,$(filter %.asm$(OBJECT_SUFFIX).o,$(ALL_OBJ)),$f: $(call fcn_obj2src,$f)\n)" >> $@
include $(BASE)obj_rules.mk
endif

# --- Object Files ---
# C++ (.cpp)
%.cpp$(OBJECT_SUFFIX).o:
	$(eval _dir=$(dir $(subst $(OBJECT_DIR)/,,$*)))
	$(eval <=$(call fcn_obj2src,$@))
	@echo [CXX] -o $<
	@mkdir -p $(dir $@)
	@$(CCPP) $(CXXFLAGS) $(CPPFLAGS) $(CXXFLAGS-$(_dir)) $(CPPFLAGS-$(_dir))-c $(_src) -o $@
# C++ (.cc)
%.cc$(OBJECT_SUFFIX).o:
	$(eval _dir=$(dir $(subst $(OBJECT_DIR)/,,$*)))
	$(eval _src=$(call fcn_obj2src,$@))
	@echo [CXX] -o $<
	@mkdir -p $(dir $@)
	@$(CCPP) $(CXXFLAGS) $(CXXFLAGS-$(_dir)) $(CPPFLAGS-$(_dir)) -c $(_src) -o $@
# C (.c)
%.c$(OBJECT_SUFFIX).o:
	$(eval _dir=$(dir $(subst $(OBJECT_DIR)/,,$*)))
	$(eval _src=$(call fcn_obj2src,$@))
	@echo [CC] -o $@
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) $(CPPFLAGS) $(CFLAGS-$(_dir)) $(CPPFLAGS-$(_dir)) -c $(_src) -o $@
# Assembly (.S)
%.S$(OBJECT_SUFFIX).o:
	$(eval _dir=$(dir $(subst $(OBJECT_DIR)/,,$*)))
	$(eval _src=$(call fcn_obj2src,$@))
	@echo [AS] -o $@
	@mkdir -p $(dir $@)
	@$(AS) $(ASFLAGS) $(ASFLAGS-$(_dir)) -o $@ $(_src)
	@$(AS) $(ASFLAGS) $(ASFLAGS-$(_dir)) -o $@.dep $(_src) -M
# Assembly (.asm)
%.asm$(OBJECT_SUFFIX).o:
	$(eval _dir=$(dir $(subst $(OBJECT_DIR)/,,$*)))
	$(eval _src=$(call fcn_obj2src,$@))
	@echo [AS] -o $@
	@mkdir -p $(dir $@)
	@$(AS) $(ASFLAGS) $(ASFLAGS-$(_dir)) -o $@ $(_src)
	@$(AS) $(ASFLAGS) $(ASFLAGS-$(_dir)) -o $@ $(_src) -MD $@.dep

# --- Binaries ---
# Static Library (.a)
%.a:
	$(eval _dir=$(subst $(OBJECT_DIR),,$(dir $<)))
	@echo [AR] ru $@
	@$(RM) $@
	@$(AR) ru $@ $(OBJ-$(_dir))
# Dynamic Library (.so)
%.so:
	$(eval _dir=$(subst $(OBJECT_DIR),,$(dir $<)))
	@echo [LD] -shared -o $@
	@$(LD) $(LDFLAGS) -shared -soname $(basename $@) -o $@ $(filter %.o,$^) $(LDFLAGS-$(_dir))
# Executable (.bin)
%.bin:
	$(eval _dir=$(subst $(OBJECT_DIR),,$(dir $<)))
	@echo [LD] -o $@
	@$(ld) $(LDFLAGS) -o $@ $(OBJ-$(_dir)) $(LDFLAGS-$(_dir))
	@$(CP) $@ $(@:%.bin=%)
    

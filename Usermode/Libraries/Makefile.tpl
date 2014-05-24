# Acess2
# - Library Common Makefile
#

comma=,

LDFLAGS += -soname $(BIN)

ifeq ($(ARCH),native)
 LDFLAGS := $(LDFLAGS:-lc=-lc_acess)
endif

_LD_CMD := $(lastword $(subst -, ,$(firstword $(LD))))
ifneq ($(_LD_CMD),ld)
  LDFLAGS := $(subst -soname ,-Wl$(comma)-soname$(comma),$(LDFLAGS))
  LDFLAGS := $(subst -Map ,-Wl$(comma)-Map$(comma),$(LDFLAGS))
  LDFLAGS := $(LDFLAGS:-x=-Wl,-x)
  LDFLAGS := $(LDFLAGS:--%=-Wl,--%)
endif

_BIN := $(addprefix $(OUTPUTDIR)Libs/,$(BIN))
_XBIN := $(addprefix $(OUTPUTDIR)Libs/,$(EXTRABIN))
_OBJPREFIX := obj-$(ARCH)/

_LIBS := $(filter -l%,$(LDFLAGS))
_LIBS := $(patsubst -l%,$(OUTPUTDIR)Libs/lib%.so,$(_LIBS))

OBJ := $(addprefix $(_OBJPREFIX),$(OBJ))

UTESTS := $(patsubst TEST_%.c,%,$(wildcard TEST_*.c))
DEPFILES := $(addsuffix .dep,$(OBJ))

ifeq ($(VERBOSE),1)
V :=
else
V := @
endif

.PHONY: all clean install postbuild

all: _libs $(_BIN) $(_XBIN)

.PHONY: _libs

.PRECIOUS: .no

HEADERS := $(patsubst include_exp/%,../../include/%,$(shell find include_exp/ -name \*.h 2>/dev/null))
_libs: $(HEADERS)

../../include/%: include_exp/%
	@mkdir -p $(dir $@)
	@ln -s $(shell pwd)/$< $@

.PHONY: utest utest-build utest-run $(UTESTS:%=runtest-%)

utest: utest-build utest-run

generate_exp: $(UTESTS:%=EXP_%.txt)
	@echo > /dev/null

utest-build: $(UTESTS:%=TEST_%)
	@echo > /dev/null

utest-run: $(UTESTS:%=runtest-%)
	@echo > /dev/null

$(UTESTS:%=runtest-%): runtest-%: TEST_% EXP_%.txt
	@echo --- [TEST] $*
	@./TEST_$* | diff EXP_$*.txt -

clean:
	$(RM) $(_BIN) $(_XBIN) $(OBJ) $(_BIN).dsm $(DEPFILES) $(EXTRACLEAN)

install: all
	@echo [xCP] $(DISTROOT)/Libs/$(BIN)
	$V$(xMKDIR) $(DISTROOT)/Libs; true
	$V$(STRIP) $(_BIN) -o $(_BIN)_
	$V$(xCP) $(_BIN)_ $(DISTROOT)/Libs/$(BIN)
	$V$(RM) $(_BIN)_
ifneq ($(_XBIN),)
	$(xCP) $(_XBIN) $(DISTROOT)/Libs/
endif
#ifneq ($(INCFILES),)
#	for f in $(INCFILES); do ln -s $f $(ACESSDIR)/include/$f; done
#endif

$(_BIN): $(OBJ)
	@mkdir -p $(dir $(_BIN))
	@echo [LD] -o $(BIN) $(OBJ)
	$V$(LD) $(LDFLAGS) -o $(_BIN) $(OBJ) $(shell $(CC) -print-libgcc-file-name)
	$V$(DISASM) -D -S $(_BIN) > $(_OBJPREFIX)$(BIN).dsm

$(_OBJPREFIX)%.o: %.c
	@echo [CC] -o $@
	@mkdir -p $(dir $@)
	$V$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ -c $< -MMD -MP -MT $@ -MF $@.dep

$(_OBJPREFIX)%.o: %.cc
	@echo [CXX] -o $@
	@mkdir -p $(dir $@)
	$V$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o $@ -c $< -MMD -MP -MT $@ -MF $@.dep

$(_OBJPREFIX)%.o: %.cpp
	@echo [CXX] -o $@
	@mkdir -p $(dir $@)
	$V$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o $@ -c $< -MMD -MP -MT $@ -MF $@.dep

$(_OBJPREFIX)%.ao: %.$(ASSUFFIX)
	@echo [AS] -o $@
	@mkdir -p $(dir $@)
	$V$(AS) $(ASFLAGS) -o $@ $<
ifeq ($(ASSUFFIX),S)
	$V$(AS) $(ASFLAGS) -o $@.dep $< -M
else
	$V$(AS) $(ASFLAGS) -o $@ $< -M > $@.dep
endif

#$(OUTPUTDIR)Libs/libld-acess.so:
#	@make -C $(ACESSDIR)/Usermode/Libraries/ld-acess.so_src/
$(OUTPUTDIR)Libs/%:
	@make -C $(ACESSDIR)/Usermode/Libraries/$*_src/


obj-native/%.no: %.c
	@mkdir -p $(dir $@)
	@echo [CC Native] -o $@
	@$(NCC) -g -c $< -o $@ -Wall -std=gnu99 -MD -MP -MF $@.dep '-D_SysDebug(f,v...)=fprintf(stderr,"DEBUG "f"\n",##v)' -include stdio.h

TEST_%: obj-native/TEST_%.no obj-native/%.no
	@echo [CC Native] -o $@
	@$(NCC) -g -o $@ $^

.PRECIOUS: $(UTESTS:%=obj-native/%.no) $(UTESTS:%=obj-native/TEST_%.no)

-include $(UTESTS:%=obj-native/TEST_%.no.dep)
-include $(UTESTS:%=obj-native/%.no.dep)

-include $(DEPFILES)

# vim: ft=make

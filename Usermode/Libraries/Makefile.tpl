# Acess2
# - Library Common Makefile
#

comma=,

LDFLAGS += -soname $(BIN)

ifeq ($(ARCH),native)
 LDFLAGS := $(LDFLAGS:-lc=-lc_acess)
endif

_BIN := $(addprefix $(OUTPUTDIR)Libs/,$(BIN))
_XBIN := $(addprefix $(OUTPUTDIR)Libs/,$(EXTRABIN))
_OBJPREFIX := obj-$(ARCH)/
LDFLAGS += -Map $(_OBJPREFIX)Map.txt

_LD_CMD := $(lastword $(subst -, ,$(firstword $(LD))))
LDFLAGS := $(subst -soname ,-Wl$(comma)-soname$(comma),$(LDFLAGS))
LDFLAGS := $(subst -Map ,-Wl$(comma)-Map$(comma),$(LDFLAGS))
LDFLAGS := $(LDFLAGS:-x=-Wl,-x)
LDFLAGS := $(LDFLAGS:--%=-Wl,--%)

_LIBS := $(filter -l%,$(LDFLAGS))
_LIBS := $(patsubst -l%,$(OUTPUTDIR)Libs/lib%.so,$(_LIBS))

ifeq ($(ARCHDIR),native)
 LIBS := $(patsubst -lc,-lc_acess,$(LIBS))
 LIBS := $(patsubst -lc++,-lc++_acess,$(LIBS))
 ifneq ($(BIN),libc_acess.so)
  LIBS += -lc_acess
 endif
endif

OBJ := $(addprefix $(_OBJPREFIX),$(OBJ))

UTESTS := $(patsubst TEST_%.c,%,$(wildcard TEST_*.c))
DEPFILES := $(addsuffix .dep,$(OBJ))

ifeq ($(VERBOSE),1)
V :=
else
V := @
endif

.PHONY: all clean install postbuild utest-build utest-run generate_exp

all: _libs $(_BIN) $(_XBIN)

.PHONY: _libs

.PRECIOUS: .no

HEADERS := $(patsubst include_exp/%,../../include/%,$(shell find include_exp/ -name \*.h 2>/dev/null))
_libs: $(HEADERS)

../../include/%: include_exp/%
	@echo [LN] $@
	@mkdir -p $(dir $@)
	@ln -s $(shell pwd)/$< $@

.PHONY: utest utest-build utest-run $(UTESTS:%=runtest-%)

utest: utest-build utest-run

utest-build: $(UTESTS:%=TEST_%)

utest-run: $(UTESTS:%=runtest-%)

$(UTESTS:%=runtest-%): runtest-%: TEST_%
	@echo --- [TEST] $*
	@./TEST_$*

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

LINK_OBJS = $(PRELINK) $(OBJ)
$(_BIN): $(CRTI) $(LINK_OBJS) $(CRTN) $(CRT0S)
	@mkdir -p $(dir $(_BIN))
	@echo [LD] -o $(BIN) $(OBJ)
ifneq ($(USE_CXX_LINK),)
	$V$(CXX) $(LDFLAGS) -o $(_BIN) $(LINK_OBJS) $(LIBS)
else
	$V$(CC) $(LDFLAGS) -o $(_BIN) $(LINK_OBJS) $(LIBS)
endif
	$V$(DISASM) -C $(_BIN) > $(_OBJPREFIX)$(BIN).dsm

$(_OBJPREFIX)%.o: %.c Makefile
	@echo [CC] -o $@
	@mkdir -p $(dir $@)
	$V$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ -c $< -MMD -MP -MT $@ -MF $@.dep

$(_OBJPREFIX)%.o: %.cc Makefile
	@echo [CXX] -o $@
	@mkdir -p $(dir $@)
	$V$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o $@ -c $< -MMD -MP -MT $@ -MF $@.dep

$(_OBJPREFIX)%.o: %.cpp Makefile
	@echo [CXX] -o $@
	@mkdir -p $(dir $@)
	$V$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o $@ -c $< -MMD -MP -MT $@ -MF $@.dep

$(_OBJPREFIX)%.ao: %.$(ASSUFFIX) Makefile
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

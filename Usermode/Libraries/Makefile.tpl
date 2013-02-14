# Acess2
# - Library Common Makefile
#


_BIN := $(OUTPUTDIR)Libs/$(BIN)
_XBIN := $(addprefix $(OUTPUTDIR)Libs/,$(EXTRABIN))
_OBJPREFIX := obj-$(ARCH)/

_LIBS := $(filter -l%,$(LDFLAGS))
_LIBS := $(patsubst -l%,$(OUTPUTDIR)Libs/lib%.so,$(_LIBS))

OBJ := $(addprefix $(_OBJPREFIX),$(OBJ))

UTESTS := $(patsubst TEST_%.c,%,$(wildcard TEST_*.c))
DEPFILES := $(addsuffix .dep,$(OBJ))

.PHONY: all clean install postbuild

all: $(_BIN) $(_XBIN)

.PHONY: utest utest-build utest-run $(UTESTS:%=runtest-%)

utest: utest-build utest-run

utest-build: $(UTESTS:%=TEST_%)

utest-run: $(UTESTS:%=runtest-%)

$(UTESTS:%=runtest-%): runtest-%: TEST_%
	./TEST_$* | diff EXP_$*.txt -

clean:
	$(RM) $(_BIN) $(_XBIN) $(OBJ) $(_BIN).dsm $(DEPFILES) $(EXTRACLEAN)

install: all
	@echo [xCP] $(DISTROOT)/Libs/$(BIN)
	@$(xMKDIR) $(DISTROOT)/Libs; true
	@$(STRIP) $(_BIN) -o $(_BIN)_
	@$(xCP) $(_BIN)_ $(DISTROOT)/Libs/$(BIN)
	@$(RM) $(_BIN)_
ifneq ($(_XBIN),)
	$(xCP) $(_XBIN) $(DISTROOT)/Libs/
endif
#ifneq ($(INCFILES),)
#	for f in $(INCFILES); do ln -s $f $(ACESSDIR)/include/$f; done
#endif

$(_BIN): $(OBJ) $(_LIBS)
	@mkdir -p $(dir $(_BIN))
	@echo [LD] -o $(BIN) $(OBJ)
	@$(LD) $(LDFLAGS) -o $(_BIN) $(OBJ) $(shell $(CC) -print-libgcc-file-name)
	@$(DISASM) -D -S $(_BIN) > $(_OBJPREFIX)$(BIN).dsm

$(_OBJPREFIX)%.o: %.c
	@echo [CC] -o $@
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ -c $<
	@$(CC) -M -MP -MT $@ $(CPPFLAGS) $< -o $@.dep

$(_OBJPREFIX)%.ao: %.$(ASSUFFIX)
	@echo [AS] -o $@
	@mkdir -p $(dir $@)
	@$(AS) $(ASFLAGS) -o $@ $<
ifeq ($(ASSUFFIX),S)
	@$(AS) $(ASFLAGS) -o $@.dep $< -M
else
	@$(AS) $(ASFLAGS) -o $@ $< -M > $@.dep
endif

#$(OUTPUTDIR)Libs/libld-acess.so:
#	@make -C $(ACESSDIR)/Usermode/Libraries/ld-acess.so_src/
$(OUTPUTDIR)Libs/%:
	@make -C $(ACESSDIR)/Usermode/Libraries/$*_src/


obj-native/%.no: %.c
	@mkdir -p $(dir $@)
	$(NCC) -c $< -o $@ -MD -MP -MF $@.dep

TEST_%: obj-native/TEST_%.no obj-native/%.no
	$(NCC) -o $@ $^

-include $(UTESTS:%=obj-native/TEST_%.no.dep)
-include $(UTESTS:%=obj-native/%.no.dep)

-include $(DEPFILES)

# vim: ft=make

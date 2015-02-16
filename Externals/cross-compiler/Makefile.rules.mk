
.PHONY: all clean binutils gcc include

all: include binutils gcc

clean:
	$(RM) -rf $(BINUTILS_DIR) $(GCC_DIR) build-$(ARCH)

gcc: $(GCC_DIR) $(PREFIX)/bin/$(TARGET)-gcc

binutils: $(BINUTILS_DIR) $(PREFIX)/bin/$(TARGET)-ld

$(BINUTILS_DIR) $(GCC_DIR): %: %.tar.bz2
	tar -xf $<

$(GCC_DIR)/%: patches/gcc/%.patch
	@echo [PATCH] $@
	@tar -xf $(GCC_ARCHIVE) $@
	@patch $@ $<
$(GCC_DIR)/%: patches/gcc/%
	@echo [CP] $@
	@cp $< $@

$(BINUTILS_DIR)/%: patches/binutils/%.patch
	@echo [PATCH] $@
	@tar -xf $(BINUTILS_ARCHIVE) $@
	@patch $@ $<
$(BINUTILS_DIR)/%: patches/binutils/%
	@echo [CP] $@
	@cp $< $@



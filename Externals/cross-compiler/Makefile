
-include ../../Makefile.cfg

GCC_ARCHIVE:=$(lastword $(sort $(wildcard gcc-*.tar.bz2)))
GCC_DIR:=$(GCC_ARCHIVE:%.tar.bz2=%)
BINUTILS_ARCHIVE:=$(lastword $(sort $(wildcard binutils-*.tar.bz2)))
BINUTILS_DIR:=$(BINUTILS_ARCHIVE:%.tar.bz2=%)

BINUTILS_CHANGES := config.sub bfd/config.bfd gas/configure.tgt ld/configure.tgt ld/emulparams/acess2_i386.sh ld/Makefile.in
GCC_CHANGES := config.sub gcc/config.gcc gcc/config/acess2.h libgcc/config.host 

ARCH?=i586
PARLEVEL ?= 3

TARGET=$(ARCH)-pc-acess2
PREFIX=$(shell pwd)/cross

.PHONY: all clean binutils gcc include

all: include binutils gcc

include:
	mkdir -p $(PREFIX)
	rm -r $(PREFIX)/include
	ln -s $(ACESSDIR)/Usermode/include $(PREFIX)/include

gcc: $(GCC_DIR) $(PREFIX)/bin/$(TARGET)-gcc

binutils: $(BINUTILS_DIR) $(PREFIX)/bin/$(TARGET)-ld

clean:
	$(RM) -rf $(BINUTILS_DIR) build-*-$(ARCH)

$(BINUTILS_DIR) $(GCC_DIR): %: %.tar.bz2
	tar -xf $<
$(warning $(BINUTILS_DIR) $(GCC_DIR))

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

$(PREFIX)/bin/$(TARGET)-ld: $(addprefix $(BINUTILS_DIR)/,$(BINUTILS_CHANGES))
	@mkdir -p build-binutils-$(ARCH)
	@cd build-binutils-$(ARCH) && ../$(BINUTILS_DIR)/configure --target=$(TARGET) --prefix=$(PREFIX) --disable-nls
	@cd build-binutils-$(ARCH) && make all install -j $(PARLEVEL)

$(PREFIX)/bin/$(TARGET)-gcc:  $(addprefix $(GCC_DIR)/,$(GCC_CHANGES))
	@mkdir -p build-gcc-$(ARCH)
	cd build-gcc-$(ARCH) && PATH=$(PREFIX)/bin:$$PATH ../$(GCC_DIR)/configure --target=$(TARGET) --prefix=$(PREFIX) --disable-nls --enable-langs=c,c++ --includedir=$(ACESSDIR)/Usermode/include --libdir=$(ACESSDIR)/Usermode/Output/$(ARCH)/Lib
	@cd build-gcc-$(ARCH) && PATH=$(PREFIX)/bin:$$PATH make all-gcc all-target-libgcc install-gcc install-target-libgcc -j $(PARLEVEL)



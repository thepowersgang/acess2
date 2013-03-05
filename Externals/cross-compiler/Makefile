
-include ../../Makefile.cfg

GCC_ARCHIVE:=$(lastword $(sort $(wildcard gcc-*.tar.bz2)))
GCC_DIR:=$(GCC_ARCHIVE:%.tar.bz2=%)
BINUTILS_ARCHIVE:=$(lastword $(sort $(wildcard binutils-*.tar.bz2)))
BINUTILS_DIR:=$(BINUTILS_ARCHIVE:%.tar.bz2=%)

BINUTILS_CHANGES := config.sub bfd/config.bfd gas/configure.tgt ld/configure.tgt ld/emulparams/acess2_i386.sh ld/Makefile.in
GCC_CHANGES := config.sub gcc/config.gcc gcc/config/acess2.h libgcc/config.host 

ifeq ($(ARCH),x86)
 BFDARCH:=i586
else ifeq ($(ARCH),x86_64)
 BFDARCH:=x86_64
else
 $(error Arch $(ARCH) is not supported by magic gcc makfile)
endif

PARLEVEL ?= 1

TARGET=$(BFDARCH)-pc-acess2
PREFIX=$(shell pwd)/cross
SYSROOT=$(shell pwd)/sysroot

.PHONY: all clean binutils gcc include

all: include binutils gcc

include:
	mkdir -p $(PREFIX)
	mkdir -p $(SYSROOT)/usr
	-ln -s $(ACESSDIR)/Usermode/include $(SYSROOT)/usr/include

gcc: $(GCC_DIR) $(PREFIX)/bin/$(TARGET)-gcc

binutils: $(BINUTILS_DIR) $(PREFIX)/bin/$(TARGET)-ld

clean:
	$(RM) -rf $(BINUTILS_DIR) $(GCC_DIR) build-$(ARCH)

$(BINUTILS_DIR) $(GCC_DIR): %: %.tar.bz2
	tar -xf $<
$(warning $(BINUTILS_DIR) $(GCC_DIR))

$(GCC_DIR)/%: patches/gcc/%.patch
	@echo [PATCH] $@
	#@tar -xf $(GCC_ARCHIVE) $@
	@patch $@ $<
$(GCC_DIR)/%: patches/gcc/%
	@echo [CP] $@
	@cp $< $@

$(BINUTILS_DIR)/%: patches/binutils/%.patch
	@echo [PATCH] $@
	#@tar -xf $(BINUTILS_ARCHIVE) $@
	@patch $@ $<
$(BINUTILS_DIR)/%: patches/binutils/%
	@echo [CP] $@
	@cp $< $@

BDIR_GCC := build-$(ARCH)/gcc
BDIR_BINUTILS := build-$(ARCH)/binutils

$(BDIR_BINUTILS)/Makefile: $(addprefix $(BINUTILS_DIR)/,$(BINUTILS_CHANGES))
	@mkdir -p $(BDIR_BINUTILS)
	@cd $(BDIR_BINUTILS) && ../../$(BINUTILS_DIR)/configure --target=$(TARGET) --prefix=$(PREFIX) --disable-nls "--with-sysroot=$(SYSROOT)" --enable-shared

$(PREFIX)/bin/$(TARGET)-ld: $(BDIR_BINUTILS)/Makefile
	@make -C $(BDIR_BINUTILS) all -j $(PARLEVEL)
	@make -C $(BDIR_BINUTILS) install

$(BDIR_GCC)/Makefile: Makefile $(addprefix $(GCC_DIR)/,$(GCC_CHANGES))
	@mkdir -p $(BDIR_GCC)
	@cd $(BDIR_GCC) && PATH=$(PREFIX)/bin:$$PATH ../../$(GCC_DIR)/configure --target=$(TARGET) --prefix=$(PREFIX) --disable-nls --enable-langs=c,c++ --includedir=$(ACESSDIR)/Usermode/include "--with-sysroot=$(SYSROOT)"

$(PREFIX)/bin/$(TARGET)-gcc: $(BDIR_GCC)/Makefile
	@PATH=$(PREFIX)/bin:$$PATH make -C $(BDIR_GCC) all-gcc -j $(PARLEVEL)
	@PATH=$(PREFIX)/bin:$$PATH make -C $(BDIR_GCC) all-target-libgcc -j $(PARLEVEL)
	@make -C $(BDIR_GCC) install-gcc install-target-libgcc



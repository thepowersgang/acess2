

-include ../config.mk

GCC_ARCHIVE:=$(lastword $(sort $(wildcard gcc-*.tar.bz2)))
GCC_DIR:=$(GCC_ARCHIVE:%.tar.bz2=%)
BINUTILS_ARCHIVE:=$(lastword $(sort $(wildcard binutils-*.tar.bz2)))
BINUTILS_DIR:=$(BINUTILS_ARCHIVE:%.tar.bz2=%)

ifeq ($(GCC_ARCHIVE),)
 $(warning Unable to find a GCC archive matching gcc-*.tar.bz2)
 $(error No archive found)
endif
ifeq ($(BINUTILS_ARCHIVE),)
 $(warning Unable to find a binutils archive matching binutils-*.tar.bz2)
 $(error No archive found)
endif

BINUTILS_CHANGES := config.sub bfd/config.bfd gas/configure.tgt ld/configure.tgt ld/emulparams/acess2_i386.sh ld/emulparams/acess2_amd64.sh ld/Makefile.in
GCC_CHANGES := config.sub gcc/config.gcc gcc/config/acess2.h libgcc/config.host
# libstdc++-v3/crossconfig.m4 config/override.m4

TARGET=$(HOST)
GCC_TARGETS := gcc target-libgcc
# target-libstdc++-v3 

PREFIX := $(OUTDIR)-BUILD
BDIR_GCC := build-$(ARCH)/gcc
BDIR_BINUTILS := build-$(ARCH)/binutils

.PHONY: all clean binutils gcc include

all: include binutils gcc

include:
	mkdir -p $(PREFIX)
	mkdir -p $(SYSROOT)/usr
	ln -sf $(ACESSDIR)/Usermode/include $(SYSROOT)/usr/include
	ln -sf $(ACESSDIR)/Usermode/Output/$(ARCH)/Libs $(SYSROOT)/usr/lib

gcc: $(GCC_DIR) $(PREFIX)/bin/$(TARGET)-gcc

binutils: $(BINUTILS_DIR) $(PREFIX)/bin/$(TARGET)-ld

clean:
	$(RM) -rf $(BINUTILS_DIR) $(GCC_DIR) build-$(ARCH)

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

$(GCC_DIR)/libstdc++-v3/configure: $(GCC_DIR)/libstdc++-v3/crossconfig.m4
	cd $(GCC_DIR)/libstdc++-v3/ && autoconf

$(BDIR_BINUTILS)/Makefile: $(addprefix $(BINUTILS_DIR)/,$(BINUTILS_CHANGES))
	@mkdir -p $(BDIR_BINUTILS)
	@cd $(BDIR_BINUTILS) && ../../$(BINUTILS_DIR)/configure --target=$(TARGET) --prefix=$(PREFIX) --disable-nls "--with-sysroot=$(SYSROOT)" --enable-shared --without-docdir

$(PREFIX)/bin/$(TARGET)-ld: $(BDIR_BINUTILS)/Makefile
	@make -C $(BDIR_BINUTILS) all -j $(PARLEVEL)
	@make -C $(BDIR_BINUTILS) install

$(BDIR_GCC)/Makefile: Makefile $(addprefix $(GCC_DIR)/,$(GCC_CHANGES)) $(GCC_DIR)/libstdc++-v3/configure
	@mkdir -p $(BDIR_GCC)
	@cd $(BDIR_GCC) && PATH=$(PREFIX)/bin:$$PATH ../../$(GCC_DIR)/configure --target=$(TARGET) --prefix=$(PREFIX) --disable-nls --enable-langs=c,c++ --includedir=$(ACESSDIR)/Usermode/include "--with-sysroot=$(SYSROOT)" --without-docdir
	@echo "MAKEINFO = :" >> $(BDIR_GCC)/Makefile

$(PREFIX)/bin/$(TARGET)-gcc: $(BDIR_GCC)/Makefile
	@PATH=$(PREFIX)/bin:$$PATH make -C $(BDIR_GCC) $(GCC_TARGETS:%=all-%) -j $(PARLEVEL)
	@PATH=$(PREFIX)/bin:$$PATH make -C $(BDIR_GCC) $(GCC_TARGETS:%=install-%)



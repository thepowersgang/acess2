#
#
#
include Makefile.common.mk

GCC_TARGETS := gcc

PREFIX := $(OUTDIR)
BDIR := build-n-$(ARCH)/
BDIR_GCC := $(BDIR)gcc
BDIR_BINUTILS := $(BDIR)binutils

ENVVARS := PATH=$(OUTDIR)-BUILD/bin:$$PATH

include Makefile.rules.mk

include:
	mkdir -p $(PREFIX)

$(GCC_DIR)/libstdc++-v3/configure: $(GCC_DIR)/libstdc++-v3/crossconfig.m4
	cd $(GCC_DIR)/libstdc++-v3/ && autoconf

$(BDIR_BINUTILS)/Makefile: $(addprefix $(BINUTILS_DIR)/,$(BINUTILS_CHANGES))
	@mkdir -p $(BDIR_BINUTILS)
	@cd $(BDIR_BINUTILS) && ../../$(BINUTILS_DIR)/configure --target=$(TARGET) --prefix=$(PREFIX) --disable-nls --enable-shared --without-docdir

$(PREFIX)/bin/$(TARGET)-ld: $(BDIR_BINUTILS)/Makefile
	@make -C $(BDIR_BINUTILS) all -j $(PARLEVEL)
	@make -C $(BDIR_BINUTILS) install

$(BDIR_GCC)/Makefile: Makefile $(addprefix $(GCC_DIR)/,$(GCC_CHANGES)) $(GCC_DIR)/libstdc++-v3/configure
	@mkdir -p $(BDIR_GCC)
	@cd $(BDIR_GCC) && $(ENVVARS) ../../$(GCC_DIR)/configure --target=$(TARGET) --prefix=$(PREFIX) --disable-nls --enable-langs=c,c++ --includedir=$(ACESSDIR)/Usermode/include --without-docdir --enable-threads=posix
	@echo "MAKEINFO = :" >> $(BDIR_GCC)/Makefile

$(PREFIX)/bin/$(TARGET)-gcc: $(BDIR_GCC)/Makefile
	@$(ENVVARS) make -C $(BDIR_GCC) $(GCC_TARGETS:%=all-%) -j $(PARLEVEL)
	@$(ENVVARS) make -C $(BDIR_GCC)libstdc++-v3/ all-target-libsupc++ -j $(PARLEVEL)
	@$(ENVVARS) make -C $(BDIR_GCC) $(GCC_TARGETS:%=install-%)



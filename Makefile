#
# Acess2 Core Makefile
#
# (Oh man! This is hacky, but beautiful at the same time, much like the
# rest of Acess)

.PHONY: all clean

SUBMAKE = $(MAKE) --no-print-directory

MODULES = FS_Ext2 FDD BochsGA IPStack NE2000 USB
USRLIBS = ld-acess.so libacess.so libgcc.so libc.so
USRAPPS = init login CLIShell cat ls mount ifconfig

ALL_MODULES = $(addprefix all-,$(MODULES))
ALL_USRLIBS = $(addprefix all-,$(USRLIBS))
ALL_USRAPPS = $(addprefix all-,$(USRAPPS))
CLEAN_MODULES = $(addprefix clean-,$(MODULES))
CLEAN_USRLIBS = $(addprefix clean-,$(USRLIBS))
CLEAN_USRAPPS = $(addprefix clean-,$(USRAPPS))
INSTALL_MODULES = $(addprefix install-,$(MODULES))
INSTALL_USRLIBS = $(addprefix install-,$(USRLIBS))
INSTALL_USRAPPS = $(addprefix install-,$(USRAPPS))

.PHONY: all clean install \
	$(ALL_MODULES) all-Kernel $(ALL_USRLIBS) $(ALL_USRAPPS) \
	$(CLEAN_MODULES) clean-Kernel $(CLEAN_USRLIBS) $(CLEAN_USRAPPS) \
	$(INSTALL_MODULES) install-Kernel $(INSTALL_USRLIBS) $(INSTALL_USRAPPS)

all:	$(ALL_MODULES) all-Kernel $(ALL_USRLIBS) $(ALL_USRAPPS)
clean:	$(CLEAN_MODULES) clean-Kernel $(CLEAN_USRLIBS) $(CLEAN_USRAPPS)
install:	$(INSTALL_MODULES) install-Kernel $(INSTALL_USRLIBS) $(INSTALL_USRAPPS)


$(ALL_MODULES): all-%:
	@echo === Module: $* && $(SUBMAKE) all -C Modules/$*
all-Kernel:
	@echo === Kernel && $(SUBMAKE) all -C Kernel
$(ALL_USRLIBS): all-%:
	@echo === User Library: $* && $(SUBMAKE) all -C Usermode/Libraries/$*_src
$(ALL_USRAPPS): all-%:
	@echo === User Application: $* && $(SUBMAKE) all -C Usermode/Applications/$*_src

$(CLEAN_MODULES): clean-%:
	@$(SUBMAKE) clean -C Modules/$*
clean-Kernel:
	@$(SUBMAKE) clean -C Kernel
$(CLEAN_USRLIBS): clean-%:
	@$(SUBMAKE) clean -C Usermode/Libraries/$*_src
$(CLEAN_USRAPPS): clean-%:
	@$(SUBMAKE) clean -C Usermode/Applications/$*_src

$(INSTALL_MODULES): install-%:
	@$(SUBMAKE) install -C Modules/$*
install-Kernel:
	@$(SUBMAKE) install -C Kernel
$(INSTALL_USRLIBS): install-%:
	@$(SUBMAKE) install -C Usermode/Libraries/$*_src
$(INSTALL_USRAPPS): install-%:
	@$(SUBMAKE) install -C Usermode/Applications/$*_src

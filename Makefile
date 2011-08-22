#
# Acess2 Core Makefile
#
# (Oh man! This is hacky, but beautiful at the same time, much like the
# rest of Acess)

-include Makefile.cfg

.PHONY: all clean

SUBMAKE = $(MAKE) --no-print-directory

USRLIBS := crt0.o acess.ld ld-acess.so libgcc.so libc.so
USRLIBS += libreadline.so libnet.so liburi.so
USRLIBS += libaxwin2.so libimage_sif.so

USRAPPS := init login CLIShell cat ls mount
USRAPPS += bomb
#USRAPPS += pcidump
USRAPPS += ifconfig ping telnet irc
USRAPPS += axwin2

ALL_DYNMODS = $(addprefix all-,$(DYNMODS))
ALL_MODULES := $(addprefix all-,$(MODULES))
ALL_USRLIBS := $(addprefix all-,$(USRLIBS))
ALL_USRAPPS := $(addprefix all-,$(USRAPPS))
CLEAN_DYNMODS := $(addprefix clean-,$(DYNMODS))
CLEAN_MODULES := $(addprefix clean-,$(MODULES))
CLEAN_USRLIBS := $(addprefix clean-,$(USRLIBS))
CLEAN_USRAPPS := $(addprefix clean-,$(USRAPPS))
INSTALL_DYNMODS := $(addprefix install-,$(DYNMODS))
INSTALL_MODULES := $(addprefix install-,$(MODULES))
INSTALL_USRLIBS := $(addprefix install-,$(USRLIBS))
INSTALL_USRAPPS := $(addprefix install-,$(USRAPPS))
AI_DYNMODS := $(addprefix allinstall-,$(DYNMODS))
AI_MODULES := $(addprefix allinstall-,$(MODULES))
AI_USRLIBS := $(addprefix allinstall-,$(USRLIBS))
AI_USRAPPS := $(addprefix allinstall-,$(USRAPPS))

.PHONY: all clean install \
	$(ALL_MODULES) all-Kernel $(ALL_USRLIBS) $(ALL_USRAPPS) \
	$(AI_MODULES) allinstall-Kernel $(AI_USRLIBS) $(AI_USRAPPS) \
	$(CLEAN_MODULES) clean-Kernel $(CLEAN_USRLIBS) $(CLEAN_USRAPPS) \
	$(INSTALL_MODULES) install-Kernel $(INSTALL_USRLIBS) $(INSTALL_USRAPPS)

kmode:	$(AI_MODULES) $(AI_DYNMODS) allinstall-Kernel

all-user: $(ALL_USRLIBS) $(ALL_USRAPPS)
clean-user: $(CLEAN_USRLIBS) $(CLEAN_USRAPPS)

all:	$(ALL_DYNMODS) $(ALL_MODULES) all-Kernel $(ALL_USRLIBS) $(ALL_USRAPPS)
all-install:	$(AI_DYNMODS) $(AI_MODULES) allinstall-Kernel $(AI_USRLIBS) $(AI_USRAPPS)
clean:	$(CLEAN_DYNMODS) $(CLEAN_MODULES) clean-Kernel $(CLEAN_USRLIBS) $(CLEAN_USRAPPS)
install:	install-Filesystem $(INSTALL_DYNMODS) $(INSTALL_MODULES) install-Kernel $(INSTALL_USRLIBS) $(INSTALL_USRAPPS)

# Compile Only
$(ALL_DYNMODS): all-%:
	+@echo === Dynamic Module: $* && BUILDTYPE=dynamic $(SUBMAKE) all -C Modules/$*
$(ALL_MODULES): all-%:
	+@echo === Module: $* && BUILDTYPE=static $(SUBMAKE) all -C Modules/$*
all-Kernel:
	+@echo === Kernel && $(SUBMAKE) all -C Kernel
$(ALL_USRLIBS): all-%:
	+@echo === User Library: $* && $(SUBMAKE) all -C Usermode/Libraries/$*_src
$(ALL_USRAPPS): all-%:
	+@echo === User Application: $* && $(SUBMAKE) all -C Usermode/Applications/$*_src

# Compile & Install
$(AI_DYNMODS): allinstall-%:
	+@echo === Dynamic Module: $* && BUILDTYPE=dynamic $(SUBMAKE) all install -C Modules/$*
$(AI_MODULES): allinstall-%:
	+@echo === Module: $* && BUILDTYPE=static $(SUBMAKE) all install -C Modules/$*
allinstall-Kernel:
	+@echo === Kernel && $(SUBMAKE) all install -C Kernel
$(AI_USRLIBS): allinstall-%:
	+@echo === User Library: $* && $(SUBMAKE) all install -C Usermode/Libraries/$*_src
$(AI_USRAPPS): allinstall-%:
	+@echo === User Application: $* && $(SUBMAKE) all install -C Usermode/Applications/$*_src

# Clean up compilation
$(CLEAN_DYNMODS): clean-%:
	+@BUILDTYPE=dynamic $(SUBMAKE) clean -C Modules/$*
$(CLEAN_MODULES): clean-%:
	+@BUILDTYPE=static $(SUBMAKE) clean -C Modules/$*
clean-Kernel:
	+@$(SUBMAKE) clean -C Kernel
$(CLEAN_USRLIBS): clean-%:
	+@$(SUBMAKE) clean -C Usermode/Libraries/$*_src
$(CLEAN_USRAPPS): clean-%:
	+@$(SUBMAKE) clean -C Usermode/Applications/$*_src

# Install
$(INSTALL_DYNMODS): install-%:
	@BUILDTYPE=dynamic $(SUBMAKE) install -C Modules/$*
$(INSTALL_MODULES): install-%:
	@BUILDTYPE=static $(SUBMAKE) install -C Modules/$*
install-Filesystem:
	@$(SUBMAKE) install -C Usermode/Filesystem
install-Kernel:
	@$(SUBMAKE) install -C Kernel
$(INSTALL_USRLIBS): install-%:
	@$(SUBMAKE) install -C Usermode/Libraries/$*_src
$(INSTALL_USRAPPS): install-%:
	@$(SUBMAKE) install -C Usermode/Applications/$*_src

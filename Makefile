#
# Acess2 Core Makefile
#
# (Oh man! This is hacky, but beautiful at the same time, much like the
# rest of Acess)

-include Makefile.cfg

.PHONY: all clean SyscallList all-user

SUBMAKE = $(MAKE) --no-print-directory

USRLIBS := crt0.o acess.ld ld-acess.so libc.so
USRLIBS += libreadline.so libnet.so liburi.so libpsocket.so
USRLIBS += libimage_sif.so

USRAPPS := init login CLIShell cat ls mount
USRAPPS += bomb lspci
USRAPPS += ip dhcpclient ping telnet irc wget telnetd
USRAPPS += axwin3 gui_ate

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
all-kmode:	$(ALL_MODULES) $(ALL_DYNMODS) all-Kernel

all-user: $(ALL_USRLIBS) $(ALL_USRAPPS)
clean-user: $(CLEAN_USRLIBS) $(CLEAN_USRAPPS)

all:	SyscallList $(ALL_USRLIBS) $(ALL_USRAPPS) $(ALL_MODULES) all-Kernel $(ALL_DYNMODS)
all-install:	install-Filesystem SyscallList $(AI_USRLIBS) $(AI_USRAPPS) $(AI_MODULES) allinstall-Kernel $(AI_DYNMODS)
clean:	$(CLEAN_DYNMODS) $(CLEAN_MODULES) clean-Kernel $(CLEAN_USRLIBS) $(CLEAN_USRAPPS)
install:	install-Filesystem SyscallList $(INSTALL_USRLIBS) $(INSTALL_USRAPPS) $(INSTALL_DYNMODS) $(INSTALL_MODULES) install-Kernel

SyscallList: include/syscalls.h
include/syscalls.h: KernelLand/Kernel/Makefile KernelLand/Kernel/syscalls.lst
	@make -C KernelLand/Kernel/ include/syscalls.h

_build_dynmod := BUILDTYPE=dynamic $(SUBMAKE) -C KernelLand/Modules/
_build_stmod  := BUILDTYPE=static $(SUBMAKE) -C KernelLand/Modules/
_build_kernel := $(SUBMAKE) $1 -C KernelLand/Kernel

# Compile Only
$(ALL_DYNMODS): all-%:
	+@echo === Dynamic Module: $* && $(_build_dynmod)$* all
$(ALL_MODULES): all-%:
	+@echo === Module: $* && $(_build_stmod)$* all
all-Kernel:
	+@echo === Kernel && $(_build_kernel) all
$(ALL_USRLIBS): all-%:
	+@echo === User Library: $* && $(SUBMAKE) all -C Usermode/Libraries/$*_src
$(ALL_USRAPPS): all-%:
	+@echo === User Application: $* && $(SUBMAKE) all -C Usermode/Applications/$*_src

# Compile & Install
$(AI_DYNMODS): allinstall-%:
	+@echo === Dynamic Module: $* && $(_build_dynmod)$* all install
$(AI_MODULES): allinstall-%:
	+@echo === Module: $* && $(_build_stmod)$* all install
allinstall-Kernel:
	+@echo === Kernel && $(_build_kernel) all install
$(AI_USRLIBS): allinstall-%:
	+@echo === User Library: $* && $(SUBMAKE) all install -C Usermode/Libraries/$*_src
$(AI_USRAPPS): allinstall-%:
	+@echo === User Application: $* && $(SUBMAKE) all install -C Usermode/Applications/$*_src

# Clean up compilation
$(CLEAN_DYNMODS): clean-%:
	+@$(_build_dynmod)$* clean
$(CLEAN_MODULES): clean-%:
	+@$(_build_stmod)$* clean
clean-Kernel:
	+@$(_build_kernel) clean
$(CLEAN_USRLIBS): clean-%:
	+@$(SUBMAKE) clean -C Usermode/Libraries/$*_src
$(CLEAN_USRAPPS): clean-%:
	+@$(SUBMAKE) clean -C Usermode/Applications/$*_src

# Install
ifeq ($(ARCH),host)
install-%:
	
else
$(INSTALL_DYNMODS): install-%:
	@$(_build_dynmod)$* install
$(INSTALL_MODULES): install-%:
	@$(_build_stmod)$* install
install-Filesystem:
	@$(SUBMAKE) install -C Usermode/Filesystem
install-Kernel:
	@$(_build_kernel) install
$(INSTALL_USRLIBS): install-%:
	@$(SUBMAKE) install -C Usermode/Libraries/$*_src
$(INSTALL_USRAPPS): install-%:
	@$(SUBMAKE) install -C Usermode/Applications/$*_src
endif

#
# Acess2 Core Makefile
#
# (Oh man! This is hacky, but beautiful at the same time, much like the
# rest of Acess)

-include Makefile.cfg

.PHONY: all clean SyscallList all-user

SUBMAKE = $(MAKE) --no-print-directory

USRLIBS := crt0.o acess.ld ld-acess.so libc.so libposix.so
USRLIBS += libreadline.so libnet.so liburi.so libpsocket.so
USRLIBS += libimage_sif.so libunicode.so

EXTLIBS := 
#libspiderscript
# zlib libpng

USRAPPS := init login CLIShell cat ls mount automounter
USRAPPS += bomb lspci
USRAPPS += ip dhcpclient ping telnet irc wget telnetd
USRAPPS += axwin3 gui_ate gui_shell

define targetclasses
 AI_$1      := $$(addprefix allinstall-,$$($1))
 ALL_$1     := $$(addprefix all-,$$($1))
 CLEAN_$1   := $$(addprefix clean-,$$($1))
 INSTALL_$1 := $$(addprefix install-,$$($1))
endef

$(eval $(call targetclasses,DYNMODS))
$(eval $(call targetclasses,MODULES))
$(eval $(call targetclasses,USRLIBS))
$(eval $(call targetclasses,EXTLIBS))
$(eval $(call targetclasses,USRAPPS))

targetvars := $$(AI_$1) $$(ALL_$1) $$(CLEAN_$1) $$(INSTALL_$1)

.PHONY: all clean install \
	ai-kmode all-kmode clean-kmode install-kmode \
	ai-user all-user clean-user install-user \
	utest mtest

.PHONY: allinstall-Kernel all-Kernel clean-Kernel install-Kernel \
	$(call targetvars,DYNMODS) \
	$(call targetvars,MODULES) \
	$(call targetvars,USRLIBS) \
	$(call targetvars,EXTLIBS) \
	$(call targetvars,USRAPPS)

ai-kmode:	$(AI_MODULES) allinstall-Kernel $(AI_DYNMODS)
all-kmode:	$(ALL_MODULES) all-Kernel $(ALL_DYNMODS)
clean-kmode:	$(CLEAN_MODULES) $(CLEAN_DYNMODS) clean-Kernel
install-kmode:	$(INSTALL_MODULES) install-Kernel $(INSTALL_DYNMODS)

ai-user:	$(AI_USRLIBS) $(AI_EXTLIBS) $(AI_USRAPPS)
all-user:	$(ALL_USRLIBS) $(ALL_EXTLIBS) $(ALL_USRAPPS)
clean-user:	$(CLEAN_USRLIBS) $(CLEAN_EXTLIBS) $(CLEAN_USRAPPS)
install-user:	$(INSTALL_USRLIBS) $(INSTALL_EXTLIBS) $(INSTALL_USRAPPS)

all:	SyscallList all-user all-kmode
all-install:	install-Filesystem SyscallList ai-user ai-kmode
clean:	clean-kmode clean-user
install:	install-Filesystem SyscallList install-user install-kmode

utest: $(USRLIBS:%=utest-%)

$(USRLIBS:%=utest-%): utest-%:
	@CC=$(NCC) $(SUBMAKE) -C Usermode/Libraries/$*_src generate_exp
	@CC=$(NCC) $(SUBMAKE) -C Usermode/Libraries/$*_src utest -k

# TODO: Module tests using DiskTool and NetTest
mtest:
	@echo > /dev/null

SyscallList: include/syscalls.h
include/syscalls.h: KernelLand/Kernel/Makefile KernelLand/Kernel/syscalls.lst
	@make -C KernelLand/Kernel/ include/syscalls.h

_build_dynmod := BUILDTYPE=dynamic $(SUBMAKE) -C KernelLand/Modules/
_build_stmod  := BUILDTYPE=static $(SUBMAKE) -C KernelLand/Modules/
_build_kernel := $(SUBMAKE) -C KernelLand/Kernel

define rules
$$(ALL_$1): all-%:
	+@echo === $2 && $3 all
$$(AI_$1): allinstall-%:
	+@echo === $2 && $3 all install
$$(CLEAN_$1): clean-%:
	+@echo === $2 && $3 clean
$$(INSTALL_$1): install-%:
	+@$3 install
endef

$(eval $(call rules,DYNMODS,Dynamic Module: $$*,$(_build_dynmod)$$*))
$(eval $(call rules,MODULES,Module: $$*,$(_build_stmod)$$*))
$(eval $(call rules,USRLIBS,User Library: $$*,$(SUBMAKE) -C Usermode/Libraries/$$*_src))
$(eval $(call rules,EXTLIBS,External Library: $$*,$(SUBMAKE) -C Externals/$$*))
$(eval $(call rules,USRAPPS,User Application: $$*,$(SUBMAKE) -C Usermode/Applications/$$*_src))
all-Kernel:
	+@echo === Kernel && $(_build_kernel) all
allinstall-Kernel:
	+@echo === Kernel && $(_build_kernel) all install
clean-Kernel:
	+@$(_build_kernel) clean
install-Kernel:
	@$(_build_kernel) install
install-Filesystem:
	@$(SUBMAKE) install -C Usermode/Filesystem

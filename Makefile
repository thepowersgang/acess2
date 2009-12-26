#
# Acess2 Core Makefile
#

.PHONY: all clean

MODULES = FS_Ext2 FDD BochsGA IPStack NE2000 USB
USRLIBS = ld-acess.so libacess.so libgcc.so libc.so
USRAPPS = init login CLIShell cat ls mount ifconfig

all:
	@echo === Filesystem && $(MAKE) all --no-print-directory -C Usermode/Filesystem
	@for mod in $(MODULES); do \
	(echo === Module: $$mod && $(MAKE) all --no-print-directory -C Modules/$$mod) \
	done
	
	@echo === Kernel
	@$(MAKE) all --no-print-directory -C Kernel
	
	@for lib in $(USRLIBS); do \
	(echo === User Library: $$lib && $(MAKE) all --no-print-directory -C Usermode/Libraries/`echo $$lib`_src) \
	done
	
	@for app in $(USRAPPS); do \
	(echo === User Application: $$app && $(MAKE) all --no-print-directory -C Usermode/Applications/`echo $$app`_src) \
	done

clean:
#	@$(MAKE) clean --no-print-directory -C Usermode/Filesystem
	@for mod in $(MODULES); do \
	($(MAKE) clean --no-print-directory -C Modules/$$mod) \
	done
	
	@make clean --no-print-directory -C Kernel/
	
	@for lib in $(USRLIBS); do \
	($(MAKE) clean --no-print-directory -C Usermode/Libraries/`echo $$lib`_src) \
	done
	
	@for app in $(USRAPPS); do \
	($(MAKE) clean --no-print-directory -C Usermode/Applications/`echo $$app`_src) \
	done

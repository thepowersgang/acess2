include $(dir $(lastword $(MAKEFILE_LIST)))../../Usermode/$(TYPE)/Makefile.cfg

.PHONY: shellvars

shellvars:
	@echo '_CC="$(CC)"'
	@echo '_LD="$(LD)"'
	@echo 'LDFLAGS="$(LDFLAGS)"'
	@echo 'CFLAGS="$(CFLAGS)"'
	@echo 'LIBGCC_PATH="$(LIBGCC_PATH)"'

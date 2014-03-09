include $(dir $(lastword $(MAKEFILE_LIST)))../../Usermode/$(TYPE)/Makefile.cfg

.PHONY: shellvars

shellvars:
	@echo '_CC="$(CC)"'
	@echo '_CXX="$(CXX)"'
	@echo '_LD="$(LD)"'
	@echo 'LDFLAGS="$(LDFLAGS)"'
	@echo 'CFLAGS="$(patsubst -std=%,,$(CFLAGS))"'
	@echo 'CXXFLAGS="$(patsubst -std=%,,$(CXXFLAGS))"'
	@echo 'LIBGCC_PATH="$(LIBGCC_PATH)"'
	@echo 'CRTBEGIN="$(CRTBEGIN)"'
	@echo 'CRTEND="$(CRTEND)"'

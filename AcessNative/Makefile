
.PHONY: all clean

V ?= @

all clean:
	@$(MAKE) -C acesskernel_src $@
	@$(MAKE) -C ld-acess_src $@
	@$(MAKE) -C libacess-native.so_src $@	
	@$(MAKE) -C .. $@-user ARCH=native V=$(V)

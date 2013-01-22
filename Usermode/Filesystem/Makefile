# Acess 2
# - Usermode Filesystem Structure Generator

-include ../Makefile.cfg

DIRS  = Bin SBin Libs Modules Apps
DIRS += Conf Conf/Auth
FILES = Conf/inittab
# Conf/Auth/Users Conf/Auth/Passwords Conf/Auth/Groups

#DIRS  := $(addprefix $(DISTROOT)/,$(DIRS))
#FILES := $(addprefix $(DISTROOT)/,$(FILES))

.PHONY: all clean
.PHONY: $(DIRS) $(FILES) DISTROOT

install: DISTROOT $(DIRS) $(FILES)

clean:

DISTROOT:
	$(xMKDIR) $(DISTROOT); true

$(DIRS):
	$(xMKDIR) $(DISTROOT)/$@; true

$(FILES):
	$(xCP) $@ $(DISTROOT)/$@

force: ;

# Acess 2
# - Usermode Filesystem Structure Generator

-include ../Makefile.cfg

DIRS  = Bin SBin Libs Modules Apps
DIRS += Conf Conf/Auth
FILES = Conf/BootConf.cfg
# Conf/Auth/Users Conf/Auth/Passwords Conf/Auth/Groups

#DIRS  := $(addprefix $(DISTROOT)/,$(DIRS))
#FILES := $(addprefix $(DISTROOT)/,$(FILES))

.PHONY: all clean
.PHONY: $(DIRS) $(FILES)

install:	$(DIRS) $(FILES)

clean:

$(DIRS):
	$(xMKDIR) $(DISTROOT)/$@; true

$(FILES):
	$(xCP) $@ $(DISTROOT)/$@

force: ;

# Acess 2
# - Usermode Filesystem Structure Generator

-include ../../Makefile.cfg

DIRS  = Bin SBin Libs Modules Applications
DIRS += Conf Conf/Auth
FILES = Conf/BootConf.cfg Conf/Auth/Users Conf/Auth/Passwords Conf/Auth/Groups

#DIRS  := $(addprefix $(DISTROOT)/,$(DIRS))
#FILES := $(addprefix $(DISTROOT)/,$(FILES))

.PHONY: all clean

all:	$(DIRS) $(FILES)

clean:
	$(xRMDIR) $(DIRS)
	$(xRM) $(FILES)

$(DIRS):
	$(xMKDIR) $(DISTROOT)/$@

$(FILES):
	$(xCP) $< $(DISTROOT)/$@

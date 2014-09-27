# Acess2 Basic C Library
# Makefile

-include ../Makefile.cfg

CPPFLAGS += 
CXXFLAGS += -Wall -Werror -Wextra -std=c++11
CFLAGS   += -Wall -Werror -Wextra
ASFLAGS  +=
LDFLAGS  += 
LIBS     += -lc++

include ../Makefile.tpl

%.native: %.cpp
	$(NCXX) $< -o $@ -Wall -std=c++11 -I include_exp/

TEST_cprintf.native: include_exp/cxxextras_printf


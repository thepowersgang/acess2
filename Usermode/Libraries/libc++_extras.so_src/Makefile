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

EXP_%.txt: TEST_%.native
	./$< > $@
	rm $<

%.native: %.cpp
	$(NCXX) $< -o $@ -Wall -std=c++11 -I include_exp/

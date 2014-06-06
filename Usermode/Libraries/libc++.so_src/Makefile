# Acess2 C++ Library
# Makefile

-include ../Makefile.cfg

CPPFLAGS += 
CFLAGS   += -Wall -Werror -Wextra
CXXFLAGS += -Wall -Werror -Wextra
ASFLAGS  +=
LDFLAGS  += -nostdlib
PRELINK  := $(CRTI) $(CRTBEGIN) $(OUTPUTDIR)/Libs/crt0S.o
LIBS     += -lc $(LIBGCC_PATH) $(CRTEND) $(CRTN)
USE_CXX_LINK := yes

OBJ  = misc.o new.o guard.o cxxabi.o typeinfo.o
OBJ += string.o
OBJ += exceptions.o exception_handling.o system_error.o
OBJ += gxx_personality.o
DEPFILES := $(OBJ:%.o=%.d)
BIN = libc++.so
ifeq ($(ARCHDIR),native)
 BIN = libc++_acess.so
endif

include ../Makefile.tpl

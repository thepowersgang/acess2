# Acess2 C++ Library
# Makefile

-include ../Makefile.cfg

CPPFLAGS += 
CFLAGS   += -Wall -Werror -Wextra
CXXFLAGS += -Wall -Werror -Wextra
ASFLAGS  +=
LDFLAGS  += -nostdlib
PRELINK  := $(CRTI) $(CRTBEGINS) $(CRT0S)
LIBS     += -lc $(LIBGCC_PATH) $(CRTENDS) $(CRTN)
USE_CXX_LINK := yes

OBJ  = misc.o new.o guard.o cxxabi.o typeinfo.o
OBJ += string.o mutex.o
OBJ += exceptions.o exception_handling.o system_error.o
OBJ += gxx_personality.o
ifeq ($(ARCHDIR),native)
# - Include libgcc_eh (separate in linux), and the linux libc (space avoids hack in Makefile.tpl)
LIBS += -lgcc_eh -l c
endif
DEPFILES := $(OBJ:%.o=%.d)
BIN = libc++.so
ifeq ($(ARCHDIR),native)
 BIN = libc++_acess.so
endif

include ../Makefile.tpl

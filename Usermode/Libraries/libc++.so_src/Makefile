# Acess2 C++ Library
# Makefile

-include ../Makefile.cfg

CPPFLAGS += 
CFLAGS   += -Wall -Werror -Wextra
CXXFLAGS += -Wall -Werror -Wextra
ASFLAGS  +=
LDFLAGS  += -Map map.txt -lc

OBJ  = misc.o new.o guard.o cxxabi.o typeinfo.o
OBJ += string.o
OBJ += exceptions.o exception_handling.o
DEPFILES := $(OBJ:%.o=%.d)
BIN = libc++.so
ifeq ($(ARCHDIR),native)
 OBJ := $(filter-out heap.o,$(OBJ))
 BIN = libc++_acess.so
endif

include ../Makefile.tpl

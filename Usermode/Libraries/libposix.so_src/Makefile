# Acess2 POSIX Emulation
# Makefile

-include ../Makefile.cfg

CPPFLAGS += 
CFLAGS   += -Werror -Wextra
ASFLAGS  +=
LDFLAGS  += -soname libposix.so -Map map.txt

OBJ  = main.o unistd.o dirent.o
DEPFILES := $(OBJ:%.o=%.d)
BIN = libposix.so

include ../Makefile.tpl


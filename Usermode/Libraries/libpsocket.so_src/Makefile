# Acess 2 - POSIX Sockets

include ../Makefile.cfg

CPPFLAGS +=
CFLAGS   += -Wall
LDFLAGS  += -lc -soname libpsocket.so
LIBS     += -lnet

OBJ = main.o getaddrinfo.o socket.o pton.o byteordering.o
BIN = libpsocket.so

include ../Makefile.tpl

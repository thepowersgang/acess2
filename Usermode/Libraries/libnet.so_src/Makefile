# Acess 2 - Networking Helpers

include ../Makefile.cfg

CPPFLAGS +=
CFLAGS   += -Wall
LDFLAGS  += -lc -soname libnet.so

OBJ = main.o address.o socket.o
OBJ += hostnames.o dns.o dns_proto.o
BIN = libnet.so

UTESTS = dns_prot

include ../Makefile.tpl

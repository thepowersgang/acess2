#
#
#

CC = gcc
AS = nasm
LD = ld
RM = rm -f

ASFLAGS = -felf
CPPFLAGS = -nostdinc -I../../include
CFLAGS = -Wall -Werror -O3 $(CPPFLAGS)
LDFLAGS = -I/Acess/Libs/ld-acess.so -L../../Libraries ../../Libraries/crt0.o -lacess

OBJ = main.o
BIN = ../init

.PHONY: all clean

all: $(BIN)

$(BIN): $(OBJ) Makefile
	$(LD) $(LDFLAGS) $(OBJ) -o $(BIN)
	cp $(BIN) /mnt/AcessHDD/Acess2/

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

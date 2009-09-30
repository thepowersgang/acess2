#
#
#

DBTYPE = tpl

CC = gcc
AS = nasm
LD = ld
RM = rm -f

ASFLAGS = -felf
CPPFLAGS = -I../../include
CFLAGS = -fno-stack-protector -Wall -Werror -O3 $(CPPFLAGS)
LDFLAGS = -I/Acess/Libs/ld-acess.so -L../../Libraries ../../Libraries/crt0.o -lacess -lgcc -lc

OBJ = main.o database_$(DBTYPE).o
BIN = ../login

.PHONY: all clean

all: $(BIN)

clean:
	$(RM) $(BIN) $(OBJ)

$(BIN): $(OBJ)
	$(LD) $(LDFLAGS) $(OBJ) -o $(BIN)
	cp $(BIN) /mnt/AcessHDD/Acess2/SBin/
	objdump -d ../login > login.dsm

%.o: %.c Makefile
	$(CC) $(CFLAGS) -c $< -o $@

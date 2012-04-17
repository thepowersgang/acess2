# Acess 2
#

include ../Makefile.cfg

CPPFLAGS +=
CFLAGS   += -Wall
LDFLAGS  += -lc -lgcc -soname libspiderscript.so --no-allow-shlib-undefined

OBJ  = main.o lex.o parse.o ast.o exports.o values.o
OBJ += ast_to_bytecode.o bytecode_gen.o bytecode_makefile.o
OBJ += exec.o exec_bytecode.o
BIN = libspiderscript.so

include ../Makefile.tpl

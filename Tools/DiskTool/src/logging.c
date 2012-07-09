/*
 * 
 */
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <acess_logging.h>

#define PUTERR(col,type)	{\
	fprintf(stderr, "\e["col"m[%-8.8s]"type" ", Ident); \
	va_list	args; va_start(args, Message);\
	vfprintf(stderr, Message, args);\
	va_end(args);\
	fprintf(stderr, "\e[0m\n"); \
}

// === CODE ===
void Log_KernelPanic(const char *Ident, const char *Message, ...) {
	PUTERR("35", "k")
	exit(-1);
}
void Log_Panic(const char *Ident, const char *Message, ...)
	PUTERR("34", "p")
void Log_Error(const char *Ident, const char *Message, ...)
	PUTERR("31", "e")
void Log_Warning(const char *Ident, const char *Message, ...)
	PUTERR("33", "w")
void Log_Notice(const char *Ident, const char *Message, ...)
	PUTERR("32", "n")
void Log_Log(const char *Ident, const char *Message, ...)
	PUTERR("37", "l")
void Log_Debug(const char *Ident, const char *Message, ...)
	PUTERR("37", "d")

void Warning(const char *Message, ...) {
	const char *Ident = "WARNING";
	PUTERR("33", "W")
}
void Log(const char *Message, ...) {
	const char *Ident = "LOGLOG";
	PUTERR("37", "L")
}

void Debug_HexDump(const char *Prefix, size_t Length, const void *Data)
{

}


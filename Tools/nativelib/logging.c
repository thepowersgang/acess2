/*
 * Acess2 libnative (Kernel Simulation Library)
 * - By John Hodge (thePowersGang)
 *
 * logging.c
 * - Logging functions
 */
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <acess_logging.h>
#include <ctype.h>
#include <inttypes.h>
#include <shortlock.h>

extern int	Threads_GetTID();

#define LOGHDR(col,type)	fprintf(stderr, "\e["col"m[%-8.8s]"type"%2i ", Ident, Threads_GetTID())
#define LOGTAIL()	fprintf(stderr, "\e[0m\n")

#define	LOG_LOCK_ACQUIRE()	do{ \
	if(!gbThreadInLog) 	SHORTLOCK(&glDebugLock); \
	gbThreadInLog ++; \
}while(0)
#define LOG_LOCK_RELEASE()	do {\
	gbThreadInLog --; \
	if(!gbThreadInLog)	SHORTREL(&glDebugLock); \
} while(0)

#define PUTERR(col,type)	{\
	LOG_LOCK_ACQUIRE(); \
	LOGHDR(col,type);\
	va_list	args; va_start(args, Message);\
	vfprintf(stderr, Message, args);\
	va_end(args);\
	LOGTAIL();\
	LOG_LOCK_RELEASE(); \
}

// === GLOBALS ===
int __thread	gbThreadInLog;
tShortSpinlock	glDebugLock;

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

void Panic(const char *Message, ...) {
	const char *Ident = "";
	PUTERR("35", "k")
	exit(-1);
}
void Warning(const char *Message, ...) {
	const char *Ident = "";
	PUTERR("33", "W")
}
void Log(const char *Message, ...) {
	const char *Ident = "";
	PUTERR("37", "L")
}
void Debug(const char *Message, ...) {
	const char *Ident = "";
	PUTERR("38", "D")
}

void Debug_HexDump(const char *Prefix, const void *Data, size_t Length)
{
	const uint8_t *data = Data;
	size_t	ofs;
	LOG_LOCK_ACQUIRE();
	fprintf(stderr, "[HexDump ]d %s: %i bytes\n", Prefix, (int)Length);
	for( ofs = 0; ofs + 16 <= Length; ofs += 16 )
	{
		fprintf(stderr, "[HexDump ]d %s:", Prefix);
		fprintf(stderr, "  %02x %02x %02x %02x %02x %02x %02x %02x",
			data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);
		data += 8;
		fprintf(stderr, "  %02x %02x %02x %02x %02x %02x %02x %02x",
			data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);
		data += 8;
		fprintf(stderr, "\n");
	}
	
	fprintf(stderr, "[HexDump ]d %s:", Prefix);
	for( ; ofs < Length; ofs ++ )
	{
		if( ofs % 8 == 0 )	fprintf(stderr, " ");
		fprintf(stderr, " %02x", data[ofs%16]);
	}
	fprintf(stderr, "\n");
	LOG_LOCK_RELEASE();
}

 int	giDebug_TraceLevel = 0;

void Debug_TraceEnter(const char *Function, const char *Format, ...)
{
	LOG_LOCK_ACQUIRE();
	//const char *Ident = "Trace";
	//LOGHDR("37","T");
	for( int i = 0; i < giDebug_TraceLevel; i ++ )
		fprintf(stderr, " ");
	fprintf(stderr, "%s: (", Function);

	va_list args;
	va_start(args, Format);
	
	int hasBeenPrev = 0;
	while(*Format)
	{
		while( *Format && isblank(*Format) )
			Format ++;
		if( !*Format )	break;
		
		char type = *Format++;
		const char *start = Format;
		while( *Format && !isblank(*Format) )
			Format ++;
		
		if(hasBeenPrev)
			fprintf(stderr, ",");
		hasBeenPrev = 1;
		
		fprintf(stderr, "%.*s=", (int)(Format-start), start);
		switch(type)
		{
		case 'p':
			fprintf(stderr, "%p", va_arg(args,const void *));
			break;
		case 's':
			fprintf(stderr, "\"%s\"", va_arg(args,const char *));
			break;
		case 'i':
			fprintf(stderr, "%i", va_arg(args,int));
			break;
		case 'x':
			fprintf(stderr, "0x%x", va_arg(args,unsigned int));
			break;
		case 'X':
			fprintf(stderr, "0x%"PRIx64, va_arg(args,uint64_t));
			break;
		default:
			va_arg(args,uintptr_t);
			fprintf(stderr, "?");
			break;
		}
	}

	va_end(args);

	fprintf(stderr, ")");
	LOGTAIL();
	giDebug_TraceLevel ++;
	LOG_LOCK_RELEASE();
}

void Debug_TraceLog(const char *Function, const char *Format, ...)
{
	LOG_LOCK_ACQUIRE();
	//const char *Ident = "Trace";
	//LOGHDR("37","T");
	
	for( int i = 0; i < giDebug_TraceLevel; i ++ )
		fprintf(stderr, " ");
	fprintf(stderr, "%s: ", Function);
	
	va_list args;
	va_start(args, Format);

	vfprintf(stderr, Format, args);
	
	va_end(args);
	LOGTAIL();
	LOG_LOCK_RELEASE();
}

void Debug_TraceLeave(const char *Function, char Type, ...)
{
	if( giDebug_TraceLevel == 0 ) {
		Log_Error("Debug", "Function %s called LEAVE without ENTER", Function);
	}
	
	LOG_LOCK_ACQUIRE();
	//const char *Ident = "Trace";
	//LOGHDR("37","T");
	
	va_list args;
	va_start(args, Type);

	if( giDebug_TraceLevel > 0 )
	{	
		giDebug_TraceLevel --;
		for( int i = 0; i < giDebug_TraceLevel; i ++ )
			fprintf(stderr, " ");
	}
	fprintf(stderr, "%s: RETURN", Function);
	switch(Type)
	{
	case '-':
		break;
	case 'i':
		fprintf(stderr, " %i", va_arg(args, int));
		break;
	case 'x':
		fprintf(stderr, " 0x%x", va_arg(args, unsigned int));
		break;
	case 'X':
		fprintf(stderr, " 0x%"PRIx64, va_arg(args,uint64_t));
		break;
	case 's':
		fprintf(stderr, " \"%s\"", va_arg(args, const char *));
		break;
	case 'p':
		fprintf(stderr, " %p", va_arg(args, const void *));
		break;
	case 'n':
		fprintf(stderr, " NULL");
		break;
	default:
		fprintf(stderr, " ?");
		break;
	}
	
	va_end(args);
	LOGTAIL();
	LOG_LOCK_RELEASE();
}


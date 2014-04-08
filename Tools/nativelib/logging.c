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

char _prn(char byte)
{
	return (' ' <= byte && byte <= 'z') ? byte : '.';
}
void Debug_HexDump(const char *Prefix, const void *Data, size_t Length)
{
	const uint8_t *data = Data;
	size_t	ofs;
	LOG_LOCK_ACQUIRE();
	fprintf(stderr, "[HexDump ]d %s: %i bytes\n", Prefix, (int)Length);
	for( ofs = 0; ofs + 16 <= Length; ofs += 16 )
	{
		const uint8_t	*d = data + ofs;
		fprintf(stderr, "[HexDump ]d %s:", Prefix);
		fprintf(stderr, "  %02x %02x %02x %02x %02x %02x %02x %02x",
			d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7]);
		fprintf(stderr, "  %02x %02x %02x %02x %02x %02x %02x %02x",
			d[8], d[9], d[10], d[11], d[12], d[13], d[14], d[15]);
		fprintf(stderr, " |%c%c%c%c""%c%c%c%c %c%c%c%c""%c%c%c%c",
			_prn(d[ 0]), _prn(d[ 1]), _prn(d[ 2]), _prn(d[ 3]),
			_prn(d[ 4]), _prn(d[ 5]), _prn(d[ 6]), _prn(d[ 7]),
			_prn(d[ 8]), _prn(d[ 9]), _prn(d[10]), _prn(d[11]),
			_prn(d[12]), _prn(d[13]), _prn(d[14]), _prn(d[15])
			);
		fprintf(stderr, "\n");
	}
	
	if( ofs < Length )
	{
		const uint8_t	*d = data + ofs;
		fprintf(stderr, "[HexDump ]d %s: ", Prefix);
		for( int i = 0; i < Length - ofs; i ++ )
		{
			if( i == 8 )	fprintf(stderr, " ");
			fprintf(stderr, " %02x", d[i]);
		}
		for( int i = Length - ofs; i < 16; i ++ )
		{
			if( i == 8 )	fprintf(stderr, " ");
			fprintf(stderr, "   ");
		}
		fprintf(stderr, " |");
		for( int i = 0; i < Length - ofs; i ++ )
		{
			if( i == 8 )	fprintf(stderr, " ");
			fprintf(stderr, "%c", _prn(d[i]));
		}
		
		fprintf(stderr, "\n");
	}
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


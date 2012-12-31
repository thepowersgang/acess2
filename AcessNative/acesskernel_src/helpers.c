/*
 * Acess2 Native Kernel
 * - Acess kernel emulation on another OS using SDL and UDP
 *
 * Kernel Main
 */
#include <arch.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/time.h>
#include <string.h>

#if 0
void LogF(const char *Fmt, ...)
{
	va_list	args;
	va_start(args, Fmt);
	vprintf(Fmt, args);
	va_end(args);
}

void Log(const char *Fmt, ...)
{
	va_list	args;
	printf("Log: ");
	va_start(args, Fmt);
	vprintf(Fmt, args);
	va_end(args);
	printf("\n");
}

void Warning(const char *Fmt, ...)
{
	va_list	args;
	printf("Warning: ");
	va_start(args, Fmt);
	vprintf(Fmt, args);
	va_end(args);
	printf("\n");
}

void Panic(const char *Format, ...)
{
	va_list	args;
	printf("Panic: ");
	va_start(args, Format);
	vprintf(Format, args);
	va_end(args);
	printf("\n");
	exit(-1);
}

void Debug_SetKTerminal(const char *Path)
{
	// Ignored, kernel debug goes to stdout instead of a virtual terminal
}
#endif

void KernelPanic_SetMode(void)
{
	// NOP - No need
}
void KernelPanic_PutChar(char ch)
{
	fprintf(stderr, "%c", ch);
}
void Debug_PutCharDebug(char ch)
{
	printf("%c", ch);
	if( ch == '\n' )
		fflush(stdout);
}
void Debug_PutStringDebug(const char *String)
{
	printf("%s", String);
	if( strchr(String, '\n') )
		fflush(stdout);
}

void *Heap_Allocate(const char *File, int Line, int ByteCount)
{
	return malloc(ByteCount);
}

void *Heap_AllocateZero(const char *File, int Line, int ByteCount)
{
	return calloc(ByteCount, 1);
}

void *Heap_Reallocate(const char *File, int Line, void *Ptr, int Bytes)
{
	return realloc(Ptr, Bytes);
}

void Heap_Deallocate(void *Ptr)
{
	free(Ptr);
}

char *Heap_StringDup(const char *File, int Line, const char *Str)
{
	return strdup(Str);
}

void Heap_Dump(void)
{
}

tPAddr MM_GetPhysAddr(tVAddr VAddr)
{
	return VAddr;	// HACK!
}

int MM_IsValidBuffer(tVAddr Base, int Size)
{
	return 1;
}

Uint MM_GetFlags(tVAddr VAddr)
{
	return 0;
}

int Modules_InitialiseBuiltin(const char *Name)
{
	return 0;	// Ignored
}

Sint64 now(void)
{
	struct timeval tv;
	struct timezone tz;
	gettimeofday(&tv, &tz);
	return tv.tv_sec * 1000 + tv.tv_usec/1000;
}

void IPStack_SendDebugText(const char *str)
{
	// nop
}

int strpos(const char *str, char ch)
{
	const char *retptr = strchr(str, ch);
	int rv = retptr ? retptr - str : -1;
	fprintf(stderr, "strpos: str = %p'%s', retptr = %p, rv = %i\n", str, str, retptr, rv);
	return rv;
}

int CheckString(const char *string)
{
	if( (intptr_t)string < 0x1000 )
		return 0;
	strlen(string);
	return 1;
}

int CheckMem(const void *buf, size_t len)
{
	if( (intptr_t)buf < 0x1000 )
		return 0;
	return 1;
}


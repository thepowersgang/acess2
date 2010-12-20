/*
 * Acess2 Native Kernel
 * - Acess kernel emulation on another OS using SDL and UDP
 *
 * Kernel Main
 */
#include <acess.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

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
	// Ignored, kernel debug goes to stdout
}

void *Heap_Allocate(int Count, const char *File, int Line)
{
	return malloc(Count);
}

tPAddr MM_GetPhysAddr(tVAddr VAddr)
{
	return VAddr;	// HACK!
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


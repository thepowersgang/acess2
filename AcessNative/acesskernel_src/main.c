/*
 * Acess2 Native Kernel
 * - Acess kernel emulation on another OS using SDL and UDP
 *
 * Kernel Main
 */
#include <acess.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
	return 0;
}

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

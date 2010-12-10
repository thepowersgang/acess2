/*
 * Acess2 Native Kernel
 * - Acess kernel emulation on another OS using SDL and UDP
 *
 * Kernel Main
 */
#include <stdio.h>
#include <stdlib.h>
#include <SDL/SDL.h>

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

int CheckMem(void *Mem, int Count)
{
	return 1;
}

int CheckString(const char *String)
{
	return 1;
}

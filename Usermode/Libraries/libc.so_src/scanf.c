/*
 * Acess2 C Library
 * - By John Hodge (thePowersGang)
 *
 * scanf.c
 * - *scanf family of functions
 */
#include <stdio.h>
#include <stdarg.h>

// === CODE ===
int _vcscanf(int (*__getc)(void *), void *h, const char *format, va_list ap)
{
	// TODO: Impliment
	return 0;
}

int _vsscanf_getc(void *h)
{
	const char	**ibuf = h;
	return *(*ibuf)++;	// Dereference, read, increment
}

int vscanf(const char *format, va_list ap)
{
	return vfscanf(stdin, format, ap);
}

int vsscanf(const char *str, const char *format, va_list ap)
{
	return _vcscanf(_vsscanf_getc, &str, format, ap);
}

int _vfscanf_getc(void *h)
{
	return fgetc(h);	// TODO: Handle -1 -> 0
}

int vfscanf(FILE *stream, const char *format, va_list ap)
{
	return _vcscanf(_vfscanf_getc, stream, format, ap);
}

int scanf(const char *format, ...)
{
	va_list	args;
	 int	rv;
	va_start(args, format);
	rv = vfscanf(stdin, format, args);
	va_end(args);
	return rv;
}
int fscanf(FILE *stream, const char *format, ...)
{
	va_list	args;
	 int	rv;
	va_start(args, format);
	rv = vfscanf(stream, format, args);
	va_end(args);
	return rv;
}
int sscanf(const char *str, const char *format, ...)
{
	va_list	args;
	 int	rv;
	va_start(args, format);
	rv = vsscanf(str, format, args);
	va_end(args);
	return rv;
}


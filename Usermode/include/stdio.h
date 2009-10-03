/*
 * AcessOS LibC
 * stdlib.h
 */
#ifndef __STDIO_H
#define __STDIO_H

#include <stdarg.h>

typedef struct sFILE	FILE;

extern int	printf(const char *format, ...);
extern int	vsprintf(char *buf, const char *format, va_list args);
extern int	sprintf(char *buf, const char *format, ...);

extern FILE	*fopen(char *file, char *mode);
extern FILE	*freopen(FILE *fp, char *file, char *mode);
extern void fclose(FILE *fp);
extern void fflush(FILE *fp);

extern size_t	fread(void *buf, size_t size, size_t n, FILE *fp);
extern size_t	fwrite(void *buf, size_t size, size_t n, FILE *fp);
extern int	fgetc(FILE *fp);
extern int	fputc(int ch, FILE *fp);

extern int	fprintf(FILE *fp, const char *format, ...);
extern int	vfprintf(FILE *fp, const char *format, va_list args);

extern FILE	*stdin;
extern FILE	*stdout;
extern FILE	*stderr;

#endif


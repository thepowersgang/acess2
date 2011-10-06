/*
 * AcessOS LibC
 * stdlib.h
 */
#ifndef __STDIO_H
#define __STDIO_H

#include <stdlib.h>
#include <stdarg.h>

/* === Types === */
typedef struct sFILE	FILE;

/* === CONSTANTS === */
#define EOF	(-1)

/* --- Standard IO --- */
extern int	printf(const char *format, ...);
extern int	vsnprintf(char *buf, size_t __maxlen, const char *format, va_list args);
extern int	vsprintf(char *buf, const char *format, va_list args);
extern int	sprintf(char *buf, const char *format, ...);
extern int	snprintf(char *buf, size_t maxlen, const char *format, ...);

extern FILE	*fopen(const char *file, const char *mode);
extern FILE	*freopen(const char *file, const char *mode, FILE *fp);
extern FILE	*fdopen(int fd, const char *modes);
extern int	fclose(FILE *fp);
extern void	fflush(FILE *fp);
extern off_t	ftell(FILE *fp);
extern int	fseek(FILE *fp, long int amt, int whence);

extern size_t	fread(void *buf, size_t size, size_t n, FILE *fp);
extern size_t	fwrite(void *buf, size_t size, size_t n, FILE *fp);
extern int	fgetc(FILE *fp);
extern int	fputc(int ch, FILE *fp);
extern int	getchar(void);
extern int	putchar(int ch);

extern int	fprintf(FILE *fp, const char *format, ...);
extern int	vfprintf(FILE *fp, const char *format, va_list args);

extern FILE	*stdin;
extern FILE	*stdout;
extern FILE	*stderr;

#endif


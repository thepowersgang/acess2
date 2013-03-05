/*
 * AcessOS LibC
 * stdlib.h
 */
#ifndef __STDIO_H
#define __STDIO_H

#include <sys/types.h>
#include <stdarg.h>
#include <stddef.h>	// size_t

/* === Types === */
typedef struct sFILE	FILE;

/* === CONSTANTS === */
#define EOF	(-1)
#define BUFSIZ	1024

#ifndef SEEK_CUR
#define SEEK_CUR	0
#define SEEK_SET	1
#define SEEK_END	(-1)
#endif

/* --- Standard IO --- */
extern int	printf(const char *format, ...);
extern int	vsnprintf(char *buf, size_t __maxlen, const char *format, va_list args);
extern int	vsprintf(char *buf, const char *format, va_list args);
extern int	sprintf(char *buf, const char *format, ...);
extern int	snprintf(char *buf, size_t maxlen, const char *format, ...);
extern void	perror(const char *s);

extern FILE	*fopen(const char *file, const char *mode);
extern FILE	*freopen(const char *file, const char *mode, FILE *fp);
extern FILE	*fdopen(int fd, const char *modes);
extern int	fclose(FILE *fp);
extern void	fflush(FILE *fp);
extern off_t	ftell(FILE *fp);
extern int	fseek(FILE *fp, long int amt, int whence);
extern void	clearerr(FILE *stream);
extern int	feof(FILE *stream);
extern int	ferror(FILE *stream);
extern int	fileno(FILE *stream);

extern size_t	fread(void *buf, size_t size, size_t n, FILE *fp);
extern size_t	fwrite(const void *buf, size_t size, size_t n, FILE *fp);
extern int	fgetc(FILE *fp);
extern char	*fgets(char *s, int size, FILE *fp);
extern int	fputc(int ch, FILE *fp);
extern int	fputs(const char *s, FILE *fp);
extern int	getchar(void);
extern int	putchar(int ch);

extern int	fprintf(FILE *fp, const char *format, ...);
extern int	vfprintf(FILE *fp, const char *format, va_list args);

// scanf
extern int	scanf(const char *format, ...);
extern int	fscanf(FILE *stream, const char *format, ...);
extern int	sscanf(const char *str, const char *format, ...);
extern int	vscanf(const char *format, va_list ap);
extern int	vsscanf(const char *str, const char *format, va_list ap);
extern int	vfscanf(FILE *stream, const char *format, va_list ap);

extern FILE	*stdin;
extern FILE	*stdout;
extern FILE	*stderr;

#endif


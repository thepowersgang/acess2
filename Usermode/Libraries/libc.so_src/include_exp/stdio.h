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

#define _IOFBF	2
#define _IOLBF	1
#define _IONBF	0

#ifdef ARCHDIR_is_native
#define printf	acess_printf
#define vsnprintf	acess_vsnprintf
#define vsprintf	acess_vsprintf
#define vprintf 	acess_vprintf
#define sprintf	acess_sprintf
#define snprintf	acess_snprintf
#define perror	acess_perror

#define fopen	acess_fopen
#define fdopen	acess_fdopen
#define freopen	acess_freopen
#define fmemopen	acess_fmemopen
#define open_memstream	acess_open_memstream
#define fdopen	acess_fdopen
#define fclose	acess_fclose
#define ftell	acess_ftell
#define fseek	acess_fseek
#define clearerr	acess_clearerr
#define feof	acess_feof
#define ferr	acess_ferr
#define fileno	acess_fileno

#define fread	acess_fread
#define fwrite	acess_fwrite
#define fgetc	acess_fgetc
#define fgets	acess_fgets
#define fputc	acess_fputc
#define fputs	acess_fputs
#define getchar	acess_getchar
#define putchar	acess_putchar

#define fprintf 	acess_fprintf
#define vfprintf	acess_vfprintf

#define scanf	acess_scanf
#define fscanf	acess_fscanf
#define sscanf	acess_sscanf
#define vscanf	acess_vscanf
#define vsscanf	acess_vsscanf
#define vfscanf	acess_vfscanf

#define stdin	acess_stdin
#define stdout	acess_stdout
#define stderr	acess_stderr
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
extern FILE	*fmemopen(void *buffer, size_t length, const char *mode);
extern FILE	*open_memstream(char **bufferptr, size_t *lengthptr);
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
#define getc(fp)	fgetc(fp)
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


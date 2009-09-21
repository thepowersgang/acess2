/*
 * AcessOS LibC
 * stdlib.h
 */
#ifndef __STDIO_H
#define __STDIO_H

typedef struct sFILE	FILE;

extern int	printf(const char *format, ...);
extern void sprintfv(char *buf, char *format, va_list args);
extern int	ssprintfv(char *format, va_list args);
extern int	sprintf(char *buf, char *format, ...);

extern FILE	*fopen(char *file, char *mode);
extern FILE	*freopen(FILE *fp, char *file, char *mode);
extern void fclose(FILE *fp);
extern void fflush(FILE *fp);

extern int	fprintf(FILE *fp, const char *format, ...);

#endif


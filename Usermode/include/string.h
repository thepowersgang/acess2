/*
 * AcessOS LibC
 * string.h
 */
#ifndef __STRING_H
#define __STRING_H

#include <stdlib.h>

// Memory
extern void *memset(void *dest, int val, size_t count);
extern void *memcpy(void *dest, const void *src, size_t count);
extern void *memmove(void *dest, const void *src, size_t count);
extern int	memcmp(const void *mem1, const void *mem2, size_t count);

// Strings
extern int	strlen(const char *string);
extern int	strcmp(const char *str1, const char *str2);
extern int	strncmp(const char *str1, const char *str2, size_t len);
extern char	*strcpy(char *dst, const char *src);
extern char	*strdup(const char *src);

#endif

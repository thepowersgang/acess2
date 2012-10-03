/*
 * AcessOS LibC
 * string.h
 */
#ifndef __STRING_H
#define __STRING_H

#include <stddef.h>

/* Strings */
extern size_t	strlen(const char *string);
extern size_t	strnlen(const char *string, size_t maxlen);
extern int	strcmp(const char *str1, const char *str2);
extern int	strncmp(const char *str1, const char *str2, size_t maxlen);
extern int	strcasecmp(const char *s1, const char *s2);
extern int	strncasecmp(const char *s1, const char *s2, size_t maxlen);
extern char	*strcpy(char *dst, const char *src);
extern char	*strncpy(char *dst, const char *src, size_t num);
extern char	*strcat(char *dst, const char *src);
extern char	*strdup(const char *src);
extern char	*strndup(const char *src, size_t length);
extern char	*strchr(const char *str, int character);
extern char	*strrchr(const char *str, int character);
extern char	*strstr(const char *str1, const char *str2);
extern size_t	strcspn(const char *haystack, const char *reject);
extern size_t	strspn(const char *haystack, const char *accept);

/* Memory */
extern void *memset(void *dest, int val, size_t count);
extern void *memcpy(void *dest, const void *src, size_t count);
extern void *memmove(void *dest, const void *src, size_t count);
extern int	memcmp(const void *mem1, const void *mem2, size_t count);
extern void	*memchr(const void *ptr, int value, size_t num);

#endif

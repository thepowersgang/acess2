/*
 * Acess2 Kernel
 * - By John Hodge (thePowersGang)
 *
 * acess_string.h
 * - Kernel-land string.h
 */
#ifndef _ACESS_STRING_H
#define _ACESS_STRING_H
#include <stdarg.h>

/**
 * \name Strings
 * \{
 */
// - stdio.h in userland
extern int	vsnprintf(char *__s, size_t __maxlen, const char *__format, va_list args);
extern int	snprintf(char *__s, size_t __n, const char *__format, ...);
extern int	sprintf(char *__s, const char *__format, ...);
extern size_t	strlen(const char *Str);
extern char	*strcpy(char *__dest, const char *__src);
extern char	*strncpy(char *__dest, const char *__src, size_t max);
extern char	*strcat(char *__dest, const char *__src);
extern char	*strncat(char *__dest, const char *__src, size_t n);
extern int	strcmp(const char *__str1, const char *__str2);
extern int	strncmp(const char *Str1, const char *Str2, size_t num);
// strdup macro is defined in heap.h
extern char	*_strdup(const char *File, int Line, const char *Str);
extern char	**str_split(const char *__str, char __ch);
extern char	*strchr(const char *__s, int __c);
extern char	*strrchr(const char *__s, int __c);
extern void	itoa(char *buf, Uint64 num, int base, int minLength, char pad);
extern int	atoi(const char *string);
extern unsigned long long	strtoull(const char *str, char **end, int base);
extern unsigned long	strtoul(const char *str, char **end, int base);
extern signed long long	strtoll(const char *str, char **end, int base);
extern signed long	strtol(const char *str, char **end, int base);

extern int	strucmp(const char *Str1, const char *Str2);
extern int	strpos(const char *Str, char Ch);
extern int	strpos8(const char *str, Uint32 search);

extern int	ParseInt(const char *string, int *Val);
extern int	ReadUTF8(const Uint8 *str, Uint32 *Val);
extern int	WriteUTF8(Uint8 *str, Uint32 Val);
extern int	ModUtil_SetIdent(char *Dest, const char *Value);
extern int	ModUtil_LookupString(const char **Array, const char *Needle);

extern Uint8	ByteSum(const void *Ptr, int Size);
extern int	Hex(char *Dest, size_t Size, const Uint8 *SourceData);
extern int	UnHex(Uint8 *Dest, size_t DestSize, const char *SourceString);
/**
 * \}
 */
#endif

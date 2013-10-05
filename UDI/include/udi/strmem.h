/**
 * \file udi_strmem.h
 */
#ifndef _UDI_STRMEM_H_
#define _UDI_STRMEM_H_

/**
 * \brief Gets the length of a C style string
 */
extern udi_size_t	udi_strlen(const char *s);

/**
 * \brief Appends to a string
 */
extern char *udi_strcat(char *s1, const char *s2);
extern char *udi_strncat(char *s1, const char *s2, udi_size_t n);

/**
 * \brief Compares Strings/Memory
 */
extern udi_sbit8_t udi_strcmp(const char *s1, const char *s2);
extern udi_sbit8_t udi_strncmp(const char *s1, const char *s2, udi_size_t n);
extern udi_sbit8_t udi_memcmp(const void *s1, const void *s2, udi_size_t n);

extern char *udi_strcpy(char *s1, const char *s2);
extern char *udi_strncpy(char *s1, const char *s2, udi_size_t n);
extern void *udi_memcpy(void *s1, const void *s2, udi_size_t n);
extern void *udi_memmove(void *s1, const void *s2, udi_size_t n);

extern char *udi_strncpy_rtrim(char *s1, const char *s2, udi_size_t n);

extern char *udi_strchr(const char *s, char c);
extern char *udi_strrchr(const char *s, char c);
extern void *udi_memchr (const void *s, udi_ubit8_t c, udi_size_t n);

extern void *udi_memset(void *s, udi_ubit8_t c, udi_size_t n);
extern udi_ubit32_t udi_strtou32(const char *s, char **endptr, int base);


extern udi_size_t udi_snprintf(char *s, udi_size_t max_bytes, const char *format, ...);
extern udi_size_t udi_vsnprintf(char *s, udi_size_t max_bytes, const char *format, va_list ap);



#endif

/*
 AcessOS 1
 Dynamic Loader
 By thePowersGang
*/
#include "common.h"

// === CODE ===
char *strcpy(char *dest, const char *src)
{
	char	*ret = dest;
	while(*src) {
		*dest = *src;
		src ++;	dest ++;
	}
	*dest = '\0';
	return ret;
}

char *strcat(char *dest, const char *src)
{
	char	*ret = dest;
	while(*dest)	dest++;
	while(*src)		*dest++ = *src++;
	*dest = '\0';
	return ret;
}

/**
 * \fn int strcmp(const char *s1, const char *s2)
 * \brief Compare two strings
 */
int strcmp(const char *s1, const char *s2)
{
	while(*s1 && *s1 == *s2) s1++,s2++;
	return *s1-*s2;
}

/**
 * \fn int strlen(const char *str)
 * \brief 
 */
int strlen(const char *str)
{
	 int	len = 0;
	while(*str)	len++,str++;
	return len;
}

/**
 * \fn int file_exists(char *filename)
 * \brief Checks if a file exists
 */
int file_exists(char *filename)
{
	 int	fd;
	 //fd = open(filename, OPENFLAG_READ);
	 fd = open(filename, 0);
	 if(fd == -1)	return 0;
	 close(fd);
	 return 1;
}

/*
 AcessOS 1
 Dynamic Loader
 By thePowersGang
*/
#include "common.h"
#include <stdint.h>

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

int memcmp(const void *p1, const void *p2, int len)
{
	const char	*b1 = p1, *b2 = p2;
	while(len --)
	{
		if(b1 != b2)	return b1 - b2;
	}
	return 0;
}

void *memcpy(void *dest, const void *src, size_t len)
{
	uint8_t	*d=dest;
	const uint8_t *s=src;
	while(len--)	*d++ = *s++;
	return dest;
}

/**
 * \fn int file_exists(char *filename)
 * \brief Checks if a file exists
 */
int file_exists(const char *filename)
{
	 int	fd;
	//fd = open(filename, OPENFLAG_READ);
	fd = open(filename, 0);
	if(fd == -1)	return 0;
	close(fd);
	return 1;
}

uint64_t __divmod64(uint64_t Num, uint64_t Den, uint64_t *Rem)
{
	uint64_t	ret = 0, add = 1;

	if( Den == 0 ) {
		if(Rem)	*Rem = 0;
		return -1;
	}

	// Find what power of two times Den is > Num
	while( Num >= Den )
	{
		Den <<= 1;
		add <<= 1;
	}

	// Search backwards
	while( add > 1 )
	{
		add >>= 1;
		Den >>= 1;
		// If the numerator is >= Den, subtract and add to return value
		if( Num >= Den )
		{
			ret += add;
			Num -= Den;
		}
	}
	if(Rem)	*Rem = Num;
	return ret;
}

#if 0
uint32_t __divmod32(uint32_t Num, uint32_t Den, uint32_t *Rem)
{
	uint32_t	ret = 0, add = 1;

	if( Den == 0 ) {
		if(Rem)	*Rem = 0;
		return -1;
	}

	// Find what power of two times Den is > Num
	while( Num >= Den )
	{
		Den <<= 1;
		add <<= 1;
	}

	// Search backwards
	while( add > 1 )
	{
		add >>= 1;
		Den >>= 1;
		// If the numerator is >= Den, subtract and add to return value
		if( Num >= Den )
		{
			ret += add;
			Num -= Den;
		}
	}
	if(Rem)	*Rem = Num;
	return ret;
}

uint64_t __udivdi3(uint64_t Num, uint64_t Den)
{
	return __divmod64(Num, Den, NULL);
}

uint64_t __umoddi3(uint64_t Num, uint64_t Den)
{
	uint64_t	ret;
	__divmod64(Num, Den, &ret);
	return ret;
}

int32_t __divsi3(int32_t Num, int32_t Den)
{
	int32_t sign = 1;
	if(Num < 0) {
		Num = -Num;
		sign = -sign;
	}
	if(Den < 0) {
		Den = -Den;
		sign = -sign;
	}
	return sign * __divmod32(Num, Den, NULL);
}

int32_t __modsi3(int32_t Num, int32_t Den)
{
	int32_t sign = 1;
	uint32_t tmp;
	if(Num < 0) {
		Num = -Num;
		sign = -sign;
	}
	if(Den < 0) {
		Den = -Den;
		sign = -sign;
	}
	__divmod32(Num, Den, &tmp);
	return ((int32_t)tmp)*sign;
}

uint32_t __udivsi3(uint32_t Num, uint32_t Den)
{
	return __divmod32(Num, Den, NULL);
}


uint32_t __umodsi3(uint32_t Num, uint32_t Den)
{
	uint32_t	ret;
	__divmod32(Num, Den, &ret);
	return ret;
}
#endif


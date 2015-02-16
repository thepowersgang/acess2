/*
 * Acess2 libiconv
 * - By John Hodge (thePowersGang)
 *
 * iconv.h
 * - External header
 */
#ifndef _ICONV_H_
#define _ICONV_H_

#include <stddef.h>

typedef void	*iconv_t;

extern iconv_t	iconv_open(const char *to, const char *from);
extern size_t	iconv(iconv_t cd, const char **inbuf, size_t *inbytesleft, char **outbuf, size_t *outbytesleft);
extern int	iconv_close(iconv_t cd);

#endif


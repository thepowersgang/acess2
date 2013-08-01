/**
 * Acess2 C Library
 * - By John Hodge (thePowersGang)
 *
 * errno.h
 * - Error values and related functions
 */
#ifndef _LIBC_ERRNO_H_
#define _LIBC_ERRNO_H_

#include <stddef.h>	// size_t

extern int	*libc_geterrno(void);
#define	errno	(*libc_geterrno())

extern int	strerror_r(int errnum, char *buf, size_t buflen);
extern char	*strerror(int errnum);

#include "errno.enum.h"

#endif

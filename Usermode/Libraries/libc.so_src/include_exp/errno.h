/**
 * Acess2 C Library
 * - By John Hodge (thePowersGang)
 *
 * errno.h
 * - Error values and related functions
 */
#ifndef _ERRNO_H_
#define _ERRNO_H_

// TODO: Fully implement errno.h, make sure it matches the kernel one

extern int	*libc_geterrno(void);
#define	errno	(*libc_geterrno())

extern const char	*strerr(int errnum);

#include "errno.enum.h"

#endif

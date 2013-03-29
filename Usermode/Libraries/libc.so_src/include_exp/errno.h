#ifndef _ERRNO_H_
#define _ERRNO_H_

// TODO: Fully implement errno.h, make sure it matches the kernel one

extern int	*libc_geterrno();
#define	errno	(*libc_geterrno())

#define strerror(_x)	"Unimplemented"

#include "errno.enum.h"

#endif

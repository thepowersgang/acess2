#ifndef _ERRNO_H_
#define _ERRNO_H_

// TODO: Fully implement errno.h, make sure it matches the kernel one

#define	errno	_errno

#define strerror(_x)	"Unimplemented"

enum
{
	EINVAL
};

#endif

#ifndef _UNISTD_H_
#define _UNISTD_H_

#define	O_RDWR	(OPENFLAG_READ|OPENFLAG_WRITE)
#define O_WR	(OPENFLAG_WRITE)
#define O_RD	(OPENFLAG_READ)
#define O_CREAT	(OPENFLAG_CREATE)
#define O_RDONLY	OPENFLAG_READ
#define O_WRONLY	OPENFLAG_WRITE
#define O_TRUNC	OPENFLAG_TRUNCATE
#define O_APPEND	OPENFLAG_APPEND

typedef intptr_t	ssize_t;

#include "acess/sys.h"


#endif


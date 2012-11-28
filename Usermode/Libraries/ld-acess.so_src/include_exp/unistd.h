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
#define O_NONBLOCK	0	// TODO: 

#include "acess/sys.h"

#define STDIN_FILENO	0
#define STDOUT_FILENO	1
#define STDERR_FILENO	2

static inline int fork(void) { return clone(CLONE_VM, 0); }
static inline int execv(const char *b,char *v[]) { return execve(b,v,NULL); }

#endif


/*
 * Acess2 POSIX Emulation Layer
 * - By John Hodge
 * 
 * unistd.h
 * - 
 */
#ifndef _UNISTD_H_
#define _UNISTD_H_

#include <stddef.h>

//! \brief flags for open(2)
#define O_WRONLY	0x01
#define O_RDONLY	0x02
#define	O_RDWR  	0x03
#define O_APPEND	0x04
#define O_CREAT 	0x08
#define O_DIRECTORY	0x10
#define O_ASYNC 	0x20
#define O_TRUNC 	0x40
#define O_NOFOLLOW	0x80	// don't follow symlinks
#define	O_EXCL  	0x100
#define O_NOCTTY	0	// unsupported
#define O_NONBLOCK	0x200
#define	O_SYNC	0	// not supported

#define STDIN_FILENO	0
#define STDOUT_FILENO	1
#define STDERR_FILENO	2

typedef signed long	ssize_t;

#include "sys/stat.h"	// mode_t

extern int	open(const char *path, int flags, ...);
extern int	creat(const char *path, mode_t mode);
extern int	close(int fd);

extern ssize_t	write(int fd, const void *buf, size_t count);
extern ssize_t	read(int fd, void *buf, size_t count);

extern int	fork(void);
extern int	execv(const char *b, char *v[]);

#endif


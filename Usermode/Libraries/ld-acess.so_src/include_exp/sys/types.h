/*
 */
#ifndef _SYS_TYPES_H
#define _SYS_TYPES_H

#include "../acess/intdefs.h"

typedef struct stat	t_fstat;

#define FD_SETSIZE	128


#define CLONE_VM	0x10

typedef unsigned int	id_t;
typedef unsigned long	pid_t;
typedef unsigned long	tid_t;
typedef signed long long int	time_t;
typedef long long int	off_t;

typedef unsigned int	uint;

typedef unsigned short	fd_set_ent_t;

/**
 * \brief fd_set for select()
 */
typedef struct
{
	fd_set_ent_t	flags[FD_SETSIZE/16];
}	fd_set;

#include "../acess/syscall_types.h"

extern void	FD_ZERO(fd_set *fdsetp);
extern void	FD_CLR(int fd, fd_set *fdsetp);
extern void	FD_SET(int fd, fd_set *fdsetp);
extern int	FD_ISSET(int fd, fd_set *fdsetp);

#include "../sys/stat.h"

#endif

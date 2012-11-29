/*
 * Acess2 Dynamic Linker
 * - By John Hodge (thePowersGang)
 *
 * acess/fd_set.h
 * - fd_set structure used by select()
 */
#ifndef _ACESS__FD_SET_H_
#define _ACESS__FD_SET_H_

#define FD_SETSIZE	128

typedef unsigned short	fd_set_ent_t;

/**
 * \brief fd_set for select()
 */
typedef struct
{
	fd_set_ent_t	flags[FD_SETSIZE/16];
}	fd_set;


extern void	FD_ZERO(fd_set *fdsetp);
extern void	FD_CLR(int fd, fd_set *fdsetp);
extern void	FD_SET(int fd, fd_set *fdsetp);
extern int	FD_ISSET(int fd, fd_set *fdsetp);

#endif

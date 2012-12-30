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


static inline void	FD_ZERO(fd_set *fdsetp)
{
	int i = FD_SETSIZE/16;
	while( i-- )
		fdsetp->flags[i] = 0;
}
static inline  void	FD_CLR(int fd, fd_set *fdsetp)
{
	if(fd < 0 || fd > FD_SETSIZE)	return;
	fdsetp->flags[fd/16] &= (fd_set_ent_t) ((~1 << (fd%16))) & 0xFFFF;
}
static inline  void	FD_SET(int fd, fd_set *fdsetp)
{
	if(fd < 0 || fd > FD_SETSIZE)	return;
	fdsetp->flags[fd/16] |= (fd_set_ent_t) (1 << (fd%16));
}
static inline  int	FD_ISSET(int fd, fd_set *fdsetp)
{
	if(fd < 0 || fd > FD_SETSIZE)	return 0;
	return !!( fdsetp->flags[fd/16] & (1<<(fd%16)) );
}

#endif

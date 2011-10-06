/*
 * Acess2 C Library
 * - By John Hodge (thePowersGang)
 *
 * select.c
 */

//#include <sys/select.h>
#include <sys/types.h>

void FD_ZERO(fd_set *fdsetp)
{
	int i = FD_SETSIZE/16;
	while( i-- )
		fdsetp->flags[i]=0;
}

void FD_CLR(int fd, fd_set *fdsetp)
{
	if(fd < 0 || fd > FD_SETSIZE)	return;
	fdsetp->flags[fd/16] &= (fd_set_ent_t) ((~1 << (fd%16))) & 0xFFFF;
}

void FD_SET(int fd, fd_set *fdsetp)
{
	if(fd < 0 || fd > FD_SETSIZE)	return;
	fdsetp->flags[fd/16] |= (fd_set_ent_t) (1 << (fd%16));
}

int FD_ISSET(int fd, fd_set *fdsetp)
{
	if(fd < 0 || fd > FD_SETSIZE)	return 0;
	return !!( fdsetp->flags[fd/16] & (1<<(fd%16)) );
}


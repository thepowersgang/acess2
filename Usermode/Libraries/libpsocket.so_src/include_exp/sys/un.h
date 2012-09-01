/*
 * Acess2 POSIX Sockets Emulation
 * - By John Hodge (thePowersGang)
 *
 * sys/un.h
 * - UNIX Domain Sockets
 */
#ifndef _LIBPSOCKET__SYS__UN_H_
#define _LIBPSOCKET__SYS__UN_H_

#define _SUN_PATH_MAX	108

// POSIX Defined
struct sockaddr_un
{
	sa_family_t	sun_family;
	char	sun_path[_SUN_PATH_MAX];
};

#endif


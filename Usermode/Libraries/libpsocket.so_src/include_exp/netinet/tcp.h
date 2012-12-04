/*
 * Acess2 POSIX Sockets Emulation
 * - By John Hodge (thePowersGang)
 *
 * netinet/tcp.h
 * - TCP Options
 */
#ifndef _NETINET__TCP_H_
#define _NETINET__TCP_H_


/**
 * \brief Values for \a option_name in setsockopt/getsockopt
 */
enum eSockOpts_TCP
{
	TCP_NODELAY
};

#endif


/*
 * Acess2 POSIX Sockets Emulation
 * - By John Hodge (thePowersGang)
 *
 * netinet/in.h
 * - ?Addressing?
 */
#ifndef _LIBPSOCKET__NETINET__IN_H_
#define _LIBPSOCKET__NETINET__IN_H_

#include <sys/socket.h>	// sa_family_t
#include <stdint.h>

typedef uint32_t	in_addr_t;

struct in_addr
{
	in_addr_t s_addr;
};

struct sockaddr_in
{
	sa_family_t	sin_family;
	uint16_t	sin_port;
	struct in_addr	sin_addr;
};

#define INADDR_ANY	0x00000000
#define INADDR_BROADCAST	0xFFFFFFFF

#define IPPORT_RESERVED	1024
#define IPPORT_UNRESERVED	0xC000

// getsockopt/setsockopt(level)
enum {
	IPPROTO_IP = 1,
	IPPROTO_ICMP,
	IPPROTO_TCP,
	IPPROTO_UDP
};

#define INET_ADDRSTRLEN 	16
#define INET6_ADDRSTRLEN	48	// linux

struct in6_addr
{
	unsigned char	s6_addr[16];
};

extern struct in6_addr	in6addr_any;

struct sockaddr_in6
{
	sa_family_t	sin6_family;
	uint16_t	sin6_port;
	uint32_t	sin6_flowinfo;
	struct in6_addr	sin6_addr;
	uint32_t	sin6_scope_id;
};

#endif


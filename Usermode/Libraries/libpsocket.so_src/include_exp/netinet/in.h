#ifndef _LIBPSOCKET__NETINET__IN_H_
#define _LIBPSOCKET__NETINET__IN_H_

#include <stdint.h>

struct in_addr
{
	unsigned long s_addr;
};

struct sockaddr_in
{
	uint16_t	sin_family;
	uint16_t	sin_port;
	struct in_addr	sin_addr;
};

struct in6_addr
{
	unsigned char	s6_addr[16];
};

struct sockaddr_in6
{
	uint16_t	sin6_family;
	uint16_t	sin6_port;
	uint32_t	sin6_flowinfo;
	struct in6_addr	sin6_addr;
	uint32_t	sin6_scope_id;
};

#endif


#ifndef _LIBPSOCKET__NETDB_H_
#define _LIBPSOCKET__NETDB_H_

#include <sys/socket.h>

#define AI_PASSIVE	0x001
#define AI_V4MAPPED	0x002
#define AI_ADDRCONFIG	0x004
#define AI_NUMERICHOST	0x008

enum
{
	EAI_SUCCESS,
	EAI_AGAIN,
	EAI_BADFLAGS,
	EAI_FAMILY,
	EAI_SOCKTTPE,
	
	EAI_ADDRFAMILY,
	EAI_FAIL,
	EAI_MEMORY,
	EAI_NODATA,
	EAI_NONAME,
	EAI_SERVICE,
	EAI_SYSTEM
};

struct addrinfo
{
	int	ai_flags;
	int	ai_family;
	int	ai_socktype;
	int	ai_protocol;
	socklen_t	ai_addrlen;
	struct sockaddr	*ai_addr;
	char	*ai_canonname;
	struct addrinfo	*ai_next;
};

extern int	getaddrinfo(const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res);
extern void	freeaddrinfo(struct addrinfo *res);
const char	*gai_strerror(int errorcode);


#endif


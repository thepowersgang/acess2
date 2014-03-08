#ifndef _LIBPSOCKET__NETDB_H_
#define _LIBPSOCKET__NETDB_H_

#include <sys/socket.h>

#ifdef __cplusplus
extern "C" {
#endif


struct hostent
{
	char	*h_name;
	char	**h_aliases;
	 int	h_addrtype;
	 int	h_length;
	char	**h_addr_list;
};
#define h_addr h_addr_list[0]	// backwards compataibility

struct netent
{
	char	*n_name;
	char	**n_aliases;
	int	n_addrtype;
	uint32_t	n_net;
};

struct protoent
{
	char	*p_name;
	char	**p_aliases;
	 int	p_proto;
};

struct servent
{
	char	*s_name;
	char	**s_aliases;
	int	s_port;
	char	*s_proto;
};

#define AI_PASSIVE	0x001
#define AI_V4MAPPED	0x002
#define AI_ADDRCONFIG	0x004
#define AI_NUMERICHOST	0x008
#define AI_NUMERICSERV	0x010

#define NI_NAMEREQD	(1<<0)
#define NI_DGRAM	(1<<1)
#define NI_NOFQDN	(1<<2)
#define	NI_NUMERICHOST	(1<<3)
#define NI_NUMERICSERV	(1<<4)

#define NI_MAXHOST	1024	// may not be posix

// Error values from gethostbyaddr/gethostbyname
enum
{
	HOST_NOT_FOUND = 1,
	NO_DATA,
	NO_RECOVERY,
	TRY_AGAIN,
};

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
	EAI_SYSTEM,
	EAI_OVERFLOW
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

extern int	getnameinfo(const struct sockaddr *sa, socklen_t salen, char *host, size_t hostlen, char *serv, size_t servlen, int flags);
extern int	getaddrinfo(const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res);
extern void	freeaddrinfo(struct addrinfo *res);
const char	*gai_strerror(int errorcode);

extern struct servent	*getservbyname(const char *name, const char *proto);
extern struct servent	*getservbyport(int port, const char *proto);

extern struct hostent	*gethostbyname(const char *name);
extern struct hostent	*gethostbyaddr(const void *addr, socklen_t len, int type);

extern void	setservent(int stayopen);
extern struct servent	*getservent(void);
extern void	enservent(void);

#ifdef __cplusplus
}
#endif

#endif


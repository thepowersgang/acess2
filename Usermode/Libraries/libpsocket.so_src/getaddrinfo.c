/*
 * Acess2 POSIX Sockets Library
 * - By John Hodge (thePowersGang)
 *
 * getaddrinfo.c
 * - getaddrinfo/freeaddrinfo/getnameinfo/gai_strerror
 */
#include <netdb.h>
#include <netinet/in.h>
#include <net.h>	// Net_ParseAddress
#include <stdlib.h>	// malloc
#include <string.h>	// memcpy
#include <stdlib.h>	// strtol
#include <acess/sys.h>

// === TYPES ===
struct sLookupInfo {
	struct addrinfo	**ret_p;
};

// === PROTOTYPES ===
struct addrinfo *int_new_addrinfo(int af, const void *addrdata);
int int_getaddinfo_lookupcb(void *info, int addr_type, const void *addr);

// === GLOBALS ===
static const struct {
	const char *Name;
	 int	SockType;
	 int	Protocol;
	 int	Port;
} caLocalServices[] = {
	{"telnet", SOCK_STREAM, 0, 23},
	{"http", SOCK_STREAM, 0, 80},
};
static const int ciNumLocalServices = sizeof(caLocalServices)/sizeof(caLocalServices[0]);

// === CODE ===
int getaddrinfo(const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res)
{
	static const struct addrinfo	defhints = {
		.ai_family = AF_UNSPEC,
		.ai_flags = AI_V4MAPPED | AI_ADDRCONFIG
		};
	struct addrinfo	*ret = NULL;

	_SysDebug("getaddrinfo('%s','%s',%p,%p)", node, service, hints, res);

	// Error checks
	if( !node && !service )	return EAI_NONAME;
	
	if( !hints )
		hints = &defhints;

	if( !node )
	{
		if( !(hints->ai_flags & AI_PASSIVE) )
			;	// Use localhost
		else
			;	// Use wildcard
	}
	else
	{
		// 1. Check if the node is an IP address
		{
			char	addrdata[16];
			int type = Net_ParseAddress(node, addrdata);
			switch(type)
			{
			case NET_ADDRTYPE_NULL:
				break;
			case NET_ADDRTYPE_IPV4:
				ret = int_new_addrinfo(AF_INET, addrdata);
				break;
			case NET_ADDRTYPE_IPV6:
				ret = int_new_addrinfo(AF_INET6, addrdata);
				break;
			default:
				_SysDebug("getaddrinfo: Unknown address type %i", type);
				return 1;
			}
		}
		
		// 2. Check for a DNS name
		// - No luck with above, and hints->ai_flags doesn't have AI_NUMERICHOST set
		if( !ret && !(hints->ai_flags & AI_NUMERICHOST) )
		{
			// Just does a basic A record lookup
			// TODO: Support SRV records
			// TODO: Ensure that CNAMEs are handled correctly
			struct sLookupInfo info = {
				.ret_p = &ret,
				};
			if( Net_Lookup_Addrs(node, &info, int_getaddinfo_lookupcb) ) {
				// Lookup failed, quick return
				return EAI_NONAME;
			}
		}
		
		// 3. No Match, return sad
		if( !ret )
		{
			return EAI_NONAME;
		}
	}

	int default_socktype = hints->ai_socktype;
	int default_protocol = hints->ai_protocol;
	int default_port = 0;
	
	#if 0
	if( default_protocol == 0 )
	{
		switch(default_socktype)
		{
		case SOCK_DGRAM:
			default_protocol = 
		}
	}
	#endif
	
	// Convert `node` into types
	if( service )
	{
		const char *end;
		
		default_port = strtol(service, (char**)&end, 0);
		if( *end != '\0' && (hints->ai_flags & AI_NUMERICSERV) )
		{
			return EAI_NONAME;
		}
		
		if( *end != '\0' )
		{
			// TODO: Read something like /Acess/Conf/services
			for( int i = 0; i < ciNumLocalServices; i ++ )
			{
				if( strcmp(service, caLocalServices[i].Name) == 0 ) {
					end = service + strlen(service);
					default_socktype = caLocalServices[i].SockType;
					default_protocol = caLocalServices[i].Protocol;
					default_port = caLocalServices[i].Port;
					break;
				}
			}
		}
		
		if( *end != '\0' )
		{
			_SysDebug("getaddrinfo: TODO Service translation");
		}
	}

	for( struct addrinfo *ai = ret; ai; ai = ai->ai_next)
	{
		struct sockaddr_in	*in = (void*)ai->ai_addr;
		struct sockaddr_in6	*in6 = (void*)ai->ai_addr;
		
		// Check ai_socktype/ai_protocol
		// TODO: Do both of these need to be zero for defaults to apply?
		if( ai->ai_socktype == 0 )
			ai->ai_socktype = default_socktype;
		if( ai->ai_protocol == 0 )
			ai->ai_protocol = default_protocol;
		
		switch(ai->ai_family)
		{
		case AF_INET:
			if( in->sin_port == 0 )
				in->sin_port = default_port;
			_SysDebug("%p: IPv4 [%s]:%i %i,%i",
				ai, Net_PrintAddress(4, &in->sin_addr),
				in->sin_port, ai->ai_socktype, ai->ai_protocol);
			break;
		case AF_INET6:
			if( in6->sin6_port == 0 )
				in6->sin6_port = default_port;
			_SysDebug("%p: IPv6 [%s]:%i %i,%i",
				ai, Net_PrintAddress(6, &in6->sin6_addr),
				in6->sin6_port, ai->ai_socktype, ai->ai_protocol);
			break;
		default:
			_SysDebug("getaddrinfo: Unknown address family %i (setting port)", ai->ai_family);
			return 1;
		}
	}

	*res = ret;
	return 0;
}

/**
 * \brief Allocate a new zeroed addrinfo for the specified address
 */
struct addrinfo *int_new_addrinfo(int af, const void *addrdata)
{
	size_t	addrlen = 0;
	switch(af)
	{
	case AF_INET:
		addrlen = sizeof(struct sockaddr_in);
		break;
	case AF_INET6:
		addrlen = sizeof(struct sockaddr_in6);
		break;
	default:
		_SysDebug("int_new_addrinfo: ERROR - Unknown AF %i", af);
		return NULL;
	}
	struct addrinfo* ret = malloc(sizeof(struct addrinfo) + addrlen);
	ret->ai_family = af;
	ret->ai_socktype = 0;
	ret->ai_protocol = 0;
	ret->ai_addrlen = addrlen;
	ret->ai_addr = (void*)( ret + 1 );
	ret->ai_canonname = 0;
	ret->ai_next = 0;
	switch(af)
	{
	case AF_INET:
		((struct sockaddr_in*)ret->ai_addr)->sin_family = AF_INET;
		((struct sockaddr_in*)ret->ai_addr)->sin_port = 0;
		memcpy( &((struct sockaddr_in*)ret->ai_addr)->sin_addr, addrdata, 4 );
		break;
	case AF_INET6:
		((struct sockaddr_in6*)ret->ai_addr)->sin6_family = AF_INET6;
		((struct sockaddr_in6*)ret->ai_addr)->sin6_port = 0;
		memcpy( &((struct sockaddr_in6*)ret->ai_addr)->sin6_addr, addrdata, 16 );
		break;
	default:
		_SysDebug("int_new_addrinfo: BUGCHECK - Unhandled AF %i", af);
		return NULL;
	}
	return ret;
}

// Callback for getaddrinfo's call to Net_Lookup_Addrs
int int_getaddinfo_lookupcb(void *info_v, int addr_type, const void *addr)
{
	struct sLookupInfo *info = info_v;
	struct addrinfo	*ent;
	switch( addr_type )
	{
	case NET_ADDRTYPE_IPV4:
		ent = int_new_addrinfo(AF_INET, addr);
		break;
	case NET_ADDRTYPE_IPV6:
		ent = int_new_addrinfo(AF_INET6, addr);
		break;
	default:
		// Huh... unknown address type, just ignore it
		return 0;
	}
	ent->ai_next = *info->ret_p;
	*info->ret_p = ent;
	return 0;
}

void freeaddrinfo(struct addrinfo *res)
{
	while( res )
	{
		struct addrinfo *next = res->ai_next;
		free(res);
		res = next;
	}
}

int getnameinfo(const struct sockaddr *sa, socklen_t salen, char *host, size_t hostlen, char *serv, size_t servlen, int flags)
{
	// Determine hostname
	if( host )
	{
		if( hostlen == 0 )
			return EAI_OVERFLOW;
		host[0] = '\0';
		if( !(flags & NI_NUMERICHOST) )
		{
			// Handle NI_NOFQDN
		}
		// Numeric hostname or the hostname was unresolvable
		if( host[0] == '\0' )
		{
			if( (flags & NI_NAMEREQD) )
				return EAI_NONAME;
		}
	}
	
	// Determine service name
	if( serv )
	{
		if( servlen == 0 )
			return EAI_OVERFLOW;
		serv[0] = '\0';
		if( !(flags & NI_NUMERICSERV) )
		{
			// Handle NI_DGRAM
		}
		if( serv[0] == '\0' )
		{
			
		}
	}
	return EAI_SUCCESS;
}


const char *gai_strerror(int errnum)
{
	switch(errnum)
	{
	case EAI_SUCCESS: 	return "No error";
	case EAI_FAIL:  	return "Permanent resolution failure";
	default:	return "UNKNOWN";
	}
}

struct hostent *gethostbyname(const char *name)
{
	return NULL;
}


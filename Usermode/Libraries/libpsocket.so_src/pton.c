/*
 * Acess2 POSIX Sockets Library
 * - By John Hodge (thePowersGang)
 *
 * pton.c
 * - inet_pton/inet_ntop
 */
#include <netinet/in.h>
#include <net.h>	// Net_PrintAddress
#include <acess/sys.h>	// _SysDebug
#include <string.h>

// === CODE ===
int inet_pton(int af, const char *src, void *dst)
{
	_SysDebug("TODO: inet_pton");
	return -1;
}

const char *inet_ntop(int af, const void *src, char *dest, size_t len)
{
	const char *str = NULL;
	switch(af)
	{
	case AF_INET:
		str = Net_PrintAddress(4, &((const struct in_addr*)src)->s_addr);
		break;
	case AF_INET6:
		str = Net_PrintAddress(6, &((const struct in6_addr*)src)->s6_addr);
		break;
	default:
		str = NULL;
		break;
	}
	strncpy(dest, str, len);
	return dest;
}

char *inet_ntoa(struct in_addr in)
{
	return Net_PrintAddress(4, &in.s_addr);
}


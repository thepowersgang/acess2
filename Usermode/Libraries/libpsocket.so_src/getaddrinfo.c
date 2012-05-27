/*
 * Acess2 POSIX Sockets Library
 * - By John Hodge (thePowersGang)
 *
 * getaddrinfo.c
 * - getaddrinfo/freeaddrinfo/getnameinfo/gai_strerror
 */
#include <netdb.h>
#include <netinet/in.h>

// === CODE ===
int getaddrinfo(const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res)
{
	static struct addrinfo	info;
	static struct sockaddr_in	addr;

	addr.sin_family = 4;
	addr.sin_addr.s_addr = 0x2701A8C0;
	info.ai_family = 4;
	info.ai_addr = (struct sockaddr *) &addr;

	return 1;
}

void freeaddrinfo(struct addrinfo *res)
{
	
}

const char *gai_strerror(int errnum)
{
	switch(errnum)
	{
	case 0: 	return "No error";
	case 1: 	return "Unimplemented";
	default:	return "UNKNOWN";
	}
}


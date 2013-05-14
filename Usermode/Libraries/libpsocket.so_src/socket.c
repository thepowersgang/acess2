/*
 * Acess2 POSIX Sockets Library
 * - By John Hodge (thePowersGang)
 *
 * socket.c
 * - sys/socket.h calls
 */
#include <sys/socket.h>
#include <acess/sys.h>
#include <stdlib.h>	// malloc/free
#include <string.h>
#include <netinet/in.h>
#include "common.h"

#define MAXFD	32

typedef struct s_sockinfo
{
	 int	fd;
	char	domain;
	char	type;
	char	protocol;
	struct sockaddr	*local;
	struct sockaddr	*remote;
} t_sockinfo;

struct s_sockinfo	gSockInfo[MAXFD];
static int	giNumPreinit = 0;

// === CODE ===
static t_sockinfo *_GetInfo(int FD)
{
	 int	i, nInit = 0;;
	if( FD == 0 && giNumPreinit == MAXFD )
		return NULL;
	for( i = 0; i < MAXFD; i ++ )
	{
		if( gSockInfo[i].fd == FD )
			return &gSockInfo[i];
		if( gSockInfo[i].fd )
			nInit ++;
		if( FD != 0 && nInit == giNumPreinit )
			return NULL;
	}
	return NULL;
}

int socket(int domain, int type, int protocol)
{
	t_sockinfo	*si = NULL;
	
	if( domain < 0 || domain > AF_INET6 )	return -1;
	if( type < 0 || type > SOCK_RDM )	return -1;

	// Allocate an info struct
	si = _GetInfo(0);
	if( !si )	return -1;

	int fd = _SysOpen("/Devices/null", OPENFLAG_RDWR);
	if( fd == -1 )	return -1;

	giNumPreinit ++;
	si->fd = fd;
	si->domain = domain;
	si->type = type;
	si->protocol = protocol;
	si->local = NULL;
	si->remote = NULL;

	return fd;
} 

int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
	t_sockinfo	*si = _GetInfo(sockfd);;
	if( !si )	return -1;

	if( si->local ) {
		// Oops?
		return -1;
	}

	si->local = malloc( addrlen );
	memcpy(si->local, addr, addrlen);

	return 0;
}

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
	t_sockinfo	*si = _GetInfo(sockfd);;
	if( !si )	return -1;

	if( si->remote ) {
		// Oops?
		return -1;
	}

	si->remote = malloc( addrlen );
	memcpy(si->remote, addr, addrlen);

	return 0;
}

int listen(int sockfd, int backlog)
{
	// TODO: Save this to the t_sockinfo structure
	return 0;
}

void _ClearInfo(t_sockinfo *si)
{
	if( si->local ) {
		free(si->local);
		si->local = NULL;
	}
	
	if( si->remote ) {
		free(si->remote);
		si->remote = NULL;
	}
	
	si->fd = 0;
	giNumPreinit --;
}

void _CommitServer(int sockfd)
{
	t_sockinfo	*si = _GetInfo(sockfd);
	if( !si )	return ;

	const char	*file;
	
	file = "tcps";

	if( !si->local ) {
		// Um... oops?
		return ;
	}	

	uint8_t	*addrBuffer = NULL;
	size_t addrLen = 0;
	switch( si->local->sa_family )
	{
	case AF_INET:
		addrBuffer = (void*)&((struct sockaddr_in*)si->local)->sin_addr;
		addrLen = 4;
		break;
	case AF_INET6:
		addrBuffer = (void*)&((struct sockaddr_in6*)si->local)->sin6_addr;
		addrLen = 16;
		break;
	default:
		return ;
	}
	
	char	hexAddr[addrLen*2+1];
	 int	bNonZero = 0, i;
	for( i = 0; i < addrLen; i ++ ) {
		hexAddr[i*2+0] = "0123456789ABCDEF"[addrBuffer[i] >> 4];
		hexAddr[i*2+1] = "0123456789ABCDEF"[addrBuffer[i] & 15];
		if(addrBuffer[i]) bNonZero = 1;
	}
	
	char	*path;
	if( bNonZero )
		path = mkstr("/Devices/ip/routes/@%i:%s/%s", si->local->sa_family, file);
	else
		path = mkstr("/Devices/ip/*%i/%s", si->local->sa_family, file);

	_SysReopen(si->fd, path, OPENFLAG_RDWR);
	// TODO: Error-check
	
	free(path);

	// TODO: Set up socket

	_ClearInfo(si);
}

void _CommitClient(int sockfd)
{
	t_sockinfo	*si = _GetInfo(sockfd);
	if( !si )	return ;

	_ClearInfo(si);
}

int accept(int sockfd, struct sockaddr *clientaddr, socklen_t addrlen)
{
	_CommitServer(sockfd);
	 int	child;


	child = _SysOpenChild(sockfd, "", OPENFLAG_RDWR);
	if( child == -1 )	return -1;
	
	_SysIOCtl(child, 8, clientaddr);

	
	return child;
}

int recvfrom(int sockfd, void *buffer, size_t length, int flags, struct sockaddr *clientaddr, socklen_t *addrlen)
{
	_CommitClient(sockfd);
	// TODO: Determine socket type (TCP/UDP) and use a bounce-buffer for UDP
	return _SysRead(sockfd, buffer, length);
}

int recv(int sockfd, void *buffer, size_t length, int flags)
{
	_CommitClient(sockfd);
	return _SysRead(sockfd, buffer, length);
}

int sendto(int sockfd, const void *buffer, size_t length, int flags, const struct sockaddr *clientaddr, socklen_t addrlen)
{
	_CommitClient(sockfd);
	// TODO: Determine socket type (TCP/UDP) and use a bounce-buffer for UDP
	return _SysWrite(sockfd, buffer, length);
}

int send(int sockfd, void *buffer, size_t length, int flags)
{
	_CommitClient(sockfd);
	return _SysWrite(sockfd, buffer, length);
}


int setsockopt(int socket, int level, int option_name, const void *option_value, socklen_t option_len)
{
	return 0;
}

int getsockopt(int socket, int level, int option_name, void *option_value, socklen_t *option_len)
{
	return 0;
}

int getsockname(int socket, struct sockaddr *addr, socklen_t *addrlen)
{
	return 0;
}

int getpeername(int socket, struct sockaddr *addr, socklen_t *addrlen)
{
	return 0;
}


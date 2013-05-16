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
#include <errno.h>
#include "common.h"

#define MAXFD	32

#define IOCTL_TCPC_PORT	5
#define IOCTL_TCPC_HOST	6
#define IOCTL_TCPC_CONNECT	7
#define IOCTL_TCPS_PORT	5

typedef struct s_sockinfo
{
	 int	fd;
	char	domain;
	char	type;
	char	protocol;
	struct sockaddr	*local;
	struct sockaddr	*remote;
} t_sockinfo;

// === PROTOTYPES ===
void	_CommitClient(int sockfd);

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
	
	if( domain < 0 || domain > AF_INET6 ) {
		_SysDebug("socket: Domain %i invalid", domain);
		errno = EINVAL;
		return -1;
	}
	if( type < 0 || type > SOCK_RDM ) {
		_SysDebug("socket: Type %i invalid", type);
		errno = EINVAL;
		return -1;
	}

	// Allocate an info struct
	si = _GetInfo(0);
	if( !si ) {
		errno = ENOMEM;
		return -1;
	}

	int fd = _SysOpen("/Devices/null", OPENFLAG_RDWR);
	if( fd == -1 )	return -1;

	giNumPreinit ++;
	si->fd = fd;
	si->domain = domain;
	si->type = type;
	si->protocol = protocol;
	si->local = NULL;
	si->remote = NULL;
	
	_SysDebug("socket(%i,%i,%i) = %i", domain, type, protocol, fd);
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

size_t _getAddrData(const struct sockaddr *addr, const void **dataptr, int *port)
{
	size_t	addrLen = 0;
	const struct sockaddr_in	*in4 = (void*)addr;
	const struct sockaddr_in6	*in6 = (void*)addr;
	switch( addr->sa_family )
	{
	case AF_INET:
		*dataptr = &in4->sin_addr;
		addrLen = 4;
		*port = in4->sin_port;
		break;
	case AF_INET6:
		*dataptr = &in6->sin6_addr;
		addrLen = 16;
		*port = in6->sin6_port;
		break;
	default:
		_SysDebug("libpsocket _getAddrData: Unkown sa_family %i", addr->sa_family);
		return 0;
	}
	return addrLen;
}

int _OpenIf(int DestFD, const struct sockaddr *addr, const char *file, int *port)
{
	const uint8_t	*addrBuffer = NULL;
	size_t addrLen = 0;

	addrLen = _getAddrData(addr, (const void **)&addrBuffer, port);
	if( addrLen == 0 ) {
		errno = EINVAL;
		return -1;
	}

	char	hexAddr[addrLen*2+1];
	 int	bNonZero = 0;
	for( int i = 0; i < addrLen; i ++ ) {
		hexAddr[i*2+0] = "0123456789ABCDEF"[addrBuffer[i] >> 4];
		hexAddr[i*2+1] = "0123456789ABCDEF"[addrBuffer[i] & 15];
		if(addrBuffer[i]) bNonZero = 1;
	}
	hexAddr[addrLen*2] = 0;
	
	char	*path;
	if( bNonZero )
		path = mkstr("/Devices/ip/routes/@%i:%s/%s", addr->sa_family, hexAddr, file);
	else
		path = mkstr("/Devices/ip/*%i/%s", addr->sa_family, file);

	int ret = _SysReopen(DestFD, path, OPENFLAG_RDWR);
	_SysDebug("libpsocket: _SysReopen(%i, '%s') = %i", DestFD, path, ret);
	free(path);
	// TODO: Error-check?
	return ret;
}

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
	t_sockinfo	*si = _GetInfo(sockfd);
	if( !si ||  si->remote ) {
		_SysDebug("connect: FD %i already connected", sockfd);
		errno = EALREADY;
		return -1;
	}

	si->remote = malloc( addrlen );
	memcpy(si->remote, addr, addrlen);

	 int	ret = 0;
	if( si->type == SOCK_STREAM )
	{
		int lport = 0;
		const struct sockaddr	*bindaddr = (si->local ? si->local : addr);
		ret = _OpenIf(sockfd, bindaddr, "tcpc", &lport);
		if(ret == -1)
			return ret;

		if( si->local ) {
			//_SysIOCtl(sockfd, IOCTL_TCPC_LPORT, &lport);
			_SysDebug("connect: TODO - Bind to local port");
		}		

		int port;
		const void *addrdata;
		_getAddrData(addr, &addrdata, &port);
		
		_SysIOCtl(sockfd, IOCTL_TCPC_PORT, &port);
		_SysIOCtl(sockfd, IOCTL_TCPC_HOST, (void*)addrdata);
		ret = _SysIOCtl(sockfd, IOCTL_TCPC_CONNECT, NULL);
		_SysDebug("connect: :%i = %i", port, ret);
	}
	else
	{
		_SysDebug("connect: TODO - non-TCP clients (%i)", si->type);
	}
		
	_CommitClient(sockfd);

	return ret;
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

	if( !si->local ) {
		// Um... oops?
		return ;
	}	

	if( si->type != SOCK_STREAM ) {
		_SysDebug("TODO: Non-tcp servers");
		return ;
	}

	// Bind to the local address
	int	port;
	int ret = _OpenIf(sockfd, si->local, "tcps", &port);
	if( ret == -1 ) {
		return ;
	}
	
	// Bind to port
	// TODO: Set up socket
	_SysIOCtl(sockfd, IOCTL_TCPS_PORT, &port);

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

int shutdown(int socket, int how)
{
	return 0;
}


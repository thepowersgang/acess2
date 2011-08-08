/*
 * Acess2 C Library
 * - By John Hodge (thePowersGang)
 *
 * sockets.c
 * - POSIX Sockets translation/emulation
 */
#include <acess/sys.h>
#include <sys/socket.h>

#define MAX_SOCKS	64

struct {
	 int	AddrType;
	const char	*ListenFile;
	const char	*ConnectFile;
	char	RemoteAddr[16];	// If != 0, bind to a remote addr
} gaSockInfo[MAX_SOCKS]

// === CODE ===
int socket(int domain, int type, int protocol)
{
	 int	fd, addrType=0;
	const char	*listenFile, *connectFile;

	// Validation
	switch(domain)
	{
	case PF_INET:
		addrType = AF_INET;
	case PF_INET6:
		if(addrType == 0)	addrType = AF_INET6;
		switch(type)
		{
		case SOCK_STREAM:
			listenFile = "tcps";
			connectFile = "tcpc";
			break;
		case SOCK_DGRAM:
			listenFile = "udp";
			connectFile = "udp";
			break;
		default:
			errno = EINVAL;
			return -1;
		}
		break;
	default:
		errno = EINVAL;
		return -1;
	}

	fd = open("/Devices/null", 0);
	if(fd == -1) {
		return -1;
	}

	if( fd >= MAX_SOCKS ) {
		close(fd);
		errno = ENFILE;
		return -1;
	}

	gaSockInfo[fd].AddrType = 0;
	gaSockInfo[fd].ListenFile = listenFile;
	gaSockInfo[fd].ConnectFile = connectFile;
	memset( gaSockInfo[fd].RemoteAddr, 0, sizeof(gaSockInfo[fd].RemoteAddr) );

	return fd;
}

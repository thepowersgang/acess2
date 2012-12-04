/*
 * Acess2 Telnet Server (TCP server test case)
 * - By John Hodge (thePowersGang)
 *
 * main.c
 * - All
 */
#include <stddef.h>
#include <net.h>
#include <stdio.h>
#include <acess/sys.h>

// === TYPES ===
typedef struct sClient
{
	 int	Socket;
	 int	stdout;
	 int	stdin;
} tClient;

// === PROTOTYPES ===
void	EventLoop(void);
void	Server_NewClient(int FD);
void	HandleServerBoundData(tClient *Client);
void	HandleClientBoundData(tClient *Client);

// === GLOBALS ===
// --- Configuration ---
 int	giConfig_MaxClients = 5;
// --- State ---
 int	giServerFD;
tClient	*gaClients;
 int	giNumClients;

// === CODE ===
int main(int argc, char *argv[])
{
	// TODO: Configure

	// Initialise
	gaClients = calloc(giConfig_MaxClients, sizeof(tClient));

	// Open server
	{
		uint8_t	data[16];
		 int	addrtype = Net_ParseAddress("10.0.2.10", data);
		 int	port = 23;
		giServerFD = Net_OpenSocket(addrtype, data, "tcps");
		_SysIOCtl(giServerFD, 4, &port);	// Set port and start listening
	}

	// Event loop
	EventLoop();
	
	return 0;
}

void EventLoop(void)
{
	fd_set	fds;
	 int	maxfd;

	void FD_SET_MAX(fd_set *set, int fd, int *maxfd)
	{
		FD_SET(fd, set);
		if(*maxfd < fd)	*maxfd = fd;
	}

	for( ;; )
	{
		FD_ZERO(&fds);
		maxfd = 0;
		// Fill select
		FD_SET_MAX(&fds, giServerFD, &maxfd);
		for( int i = 0; i < giConfig_MaxClients; i ++ )
		{
			if( gaClients[i].Socket == 0 )	continue ;
			_SysDebug("Socket = %i, stdout = %i",
				gaClients[i].Socket, gaClients[i].stdout);
			FD_SET_MAX(&fds, gaClients[i].Socket, &maxfd);
			FD_SET_MAX(&fds, gaClients[i].stdout,  &maxfd);
		}
		
		// Select!
		_SysSelect( maxfd+1, &fds, NULL, NULL, NULL, 0 );
		
		// Check events
		if( FD_ISSET(giServerFD, &fds) )
		{
			Server_NewClient(giServerFD);
		}
		for( int i = 0; i < giConfig_MaxClients; i ++ )
		{
			if( FD_ISSET(gaClients[i].Socket, &fds) )
			{
				// Handle client data
				HandleServerBoundData(&gaClients[i]);
			}
			if( FD_ISSET(gaClients[i].stdout, &fds) )
			{
				// Handle output from terminal
				HandleClientBoundData(&gaClients[i]);
			}
		}
	}
}

void Server_NewClient(int FD)
{
	tClient	*clt;
	
	// TODO: Is this done in the IPStack?
	if( giNumClients == giConfig_MaxClients )
	{
		// Open, reject
		_SysClose( _SysOpenChild(FD, "", OPENFLAG_READ) );
		return ;
	}
	
	// Allocate client structure and open socket
	for( int i = 0; i < giConfig_MaxClients; i ++ )
	{
		if( gaClients[i].Socket == 0 ) {
			clt = &gaClients[i];
			break;
		}
	}
	// Accept the connection
	clt->Socket = _SysOpenChild(FD, "", OPENFLAG_READ|OPENFLAG_WRITE);
	giNumClients ++;
	
	// Create stdin/stdout
	clt->stdin = _SysOpen("/Devices/fifo/anon", OPENFLAG_READ|OPENFLAG_WRITE);
	clt->stdout = _SysOpen("/Devices/fifo/anon", OPENFLAG_READ|OPENFLAG_WRITE);
	
	// TODO: Arguments and envp
	{
		int fds[3] = {clt->stdin, clt->stdout, clt->stdout};
		const char	*argv[] = {NULL};
		_SysSpawn("/Acess/SBin/login", argv, argv, 3, fds, NULL);
	}
}

void HandleServerBoundData(tClient *Client)
{
	char	buf[BUFSIZ];
	 int	len;
	
	len = _SysRead(Client->Socket, buf, BUFSIZ);
	if( len <= 0 )	return ;
	_SysWrite(Client->stdin, buf, len);
}

void HandleClientBoundData(tClient *Client)
{
	char	buf[BUFSIZ];
	 int	len;
	
	len = _SysRead(Client->stdout, buf, BUFSIZ);
	if( len <= 0 )	return ;
	_SysWrite(Client->Socket, buf, len);
}


/*
 */
#include <stddef.h>
#include <net.h>
#include <unistd.h>
#include <stdio.h>

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
	giServerFD = Net_OpenSocket(0, NULL, "tcps");

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
		maxfd = 0;
		// Fill select
		FD_SET_MAX(&fds, giServerFD, &maxfd);
		for( int i = 0; i < giConfig_MaxClients; i ++ )
		{
			FD_SET_MAX(&fds, gaClients[i].Socket, &maxfd);
			FD_SET_MAX(&fds, gaClients[i].stdout,  &maxfd);
		}
		
		// Select!
		select( maxfd+1, &fds, NULL, NULL, NULL );
		
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
		close( _SysOpenChild(FD, "", O_RDWR) );
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
	clt->Socket = _SysOpenChild(FD, "", O_RDWR);
	giNumClients ++;	
	
	// Create stdin/stdout
	clt->stdin = open("/Devices/fifo", O_RDWR);
	clt->stdout = open("/Devices/fifo", O_RDWR);
	
	// TODO: Arguments and envp
	{
		int fds[3] = {clt->stdin, clt->stdout, clt->stdout};
		_SysSpawn("/Acess/SBin/login", NULL, NULL, 3, fds);
	}
}

void HandleServerBoundData(tClient *Client)
{
	char	buf[BUFSIZ];
	 int	len;
	
	len = read(Client->Socket, buf, BUFSIZ);
	write(Client->stdin, buf, len);
}

void HandleClientBoundData(tClient *Client)
{
	char	buf[BUFSIZ];
	 int	len;
	
	len = read(Client->stdout, buf, BUFSIZ);
	write(Client->Socket, buf, len);
}


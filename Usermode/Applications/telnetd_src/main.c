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
#include <stdlib.h>
#include <acess/sys.h>
#include <assert.h>

// === TYPES ===
enum eTelnetMode
{
	MODE_DATA,
	MODE_IAC,
	MODE_WILL,
	MODE_WONT,
	MODE_DO,
	MODE_DONT
};

typedef struct sClient
{
	enum eTelnetMode	Mode;
	 int	Socket;
	 int	pty;
	 int	stdin;
	 int	stdout;
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

static void FD_SET_MAX(fd_set *set, int fd, int *maxfd)
{
	FD_SET(fd, set);
	if(*maxfd < fd)	*maxfd = fd;
}

void EventLoop(void)
{
	fd_set	fds;
	 int	maxfd;

	for( ;; )
	{
		FD_ZERO(&fds);
		maxfd = 0;
		// Fill select
		FD_SET_MAX(&fds, giServerFD, &maxfd);
		for( int i = 0; i < giConfig_MaxClients; i ++ )
		{
			if( gaClients[i].Socket == 0 )	continue ;
			_SysDebug("Socket = %i, pty = %i",
				gaClients[i].Socket, gaClients[i].pty);
			FD_SET_MAX(&fds, gaClients[i].Socket, &maxfd);
			FD_SET_MAX(&fds, gaClients[i].pty,  &maxfd);
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
			if( FD_ISSET(gaClients[i].pty, &fds) )
			{
				// Handle output from terminal
				HandleClientBoundData(&gaClients[i]);
			}
		}
	}
}

void Server_NewClient(int FD)
{
	tClient	*clt = NULL;
	
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
	assert(clt);
	// Accept the connection
	clt->Socket = _SysOpenChild(FD, "", OPENFLAG_READ|OPENFLAG_WRITE);
	giNumClients ++;
	
	// Create stdin/stdout
	// TODO: Use PTYs
	clt->stdin = _SysOpen("/Devices/fifo/anon", OPENFLAG_READ|OPENFLAG_WRITE);
	clt->stdout = _SysOpen("/Devices/fifo/anon", OPENFLAG_READ|OPENFLAG_WRITE);
	
	// TODO: Arguments and envp
	{
		 int	clientfd = _SysOpen("/Devices/pts/telnetd0c", OPENFLAG_READ|OPENFLAG_WRITE);
		if(clientfd < 0) {
			perror("Unable to open login PTY");
			_SysClose(clt->Socket);
			_SysClose(clt->pty);
			clt->Socket = 0;
			return ;
		}
		int fds[3] = {clientfd, clientfd, clientfd};
		const char	*argv[] = {"login", NULL};
		_SysSpawn("/Acess/SBin/login", argv, argv, 3, fds, NULL);
	}
}

void HandleServerBoundData(tClient *Client)
{
	uint8_t	buf[BUFSIZ];
	size_t	len;
	
	len = _SysRead(Client->Socket, buf, BUFSIZ);
	if( len == 0 )	return ;
	if( len == -1 ) {
		return ;
	}
	// handle options
	 int	last_flush = 0;
	for( int i = 0; i < len; i ++ )
	{
		switch(Client->Mode)
		{
		case MODE_IAC:
			Client->Mode = MODE_DATA;
			switch(buf[i])
			{
			case 240:	// End of subnegotiation parameters
			case 241:	// Nop
				break;
			case 242:	// SYNCH
			case 243:	// NVT Break
			case 244:	// Function 'IP' (Ctrl-C)
				
				break;
			case 245:	// Function 'AO'
			case 246:	// Function 'AYT'
			case 247:	// Function 'EC'
			case 248:	// Function 'EL'
			case 249:	// GA Signal
			case 250:	// Subnegotation
				break;
			case 251:	// WILL
				Client->Mode = MODE_WILL;
				break;
			case 252:	// WONT
				Client->Mode = MODE_WILL;
				break;
			case 253:	// DO
				Client->Mode = MODE_WILL;
				break;
			case 254:	// DONT
				Client->Mode = MODE_WILL;
				break;
			case 255:	// Literal 255
				Client->Mode = MODE_DATA;
				i --;	// hacky!
				break;
			}
			break;
		case MODE_WILL:
			_SysDebug("Option %i WILL", buf[i]);
			Client->Mode = MODE_DATA;
			break;
		case MODE_WONT:
			_SysDebug("Option %i WONT", buf[i]);
			Client->Mode = MODE_DATA;
			break;
		case MODE_DO:
			_SysDebug("Option %i DO", buf[i]);
			Client->Mode = MODE_DATA;
			break;
		case MODE_DONT:
			_SysDebug("Option %i DONT", buf[i]);
			Client->Mode = MODE_DATA;
			break;
		case MODE_DATA:
			if( buf[i] == 255 )
				Client->Mode = MODE_IAC;
			else
				_SysWrite(Client->pty, buf+i, 1);
			break;
		}
	}
}

void HandleClientBoundData(tClient *Client)
{
	char	buf[BUFSIZ];
	 int	len;
	
	len = _SysRead(Client->pty, buf, BUFSIZ);
	if( len <= 0 )	return ;
	_SysWrite(Client->Socket, buf, len);
}


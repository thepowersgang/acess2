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
#include <errno.h>
#include <acess/devices/pty.h>
#include <stdbool.h>

// === TYPES ===
enum eTelnetMode
{
	MODE_DATA,
	MODE_IAC,
	MODE_WILL,
	MODE_WONT,
	MODE_DO,
	MODE_DONT,
	MODE_SUBNEG_OPTION,
	MODE_SUBNEG_DATA,
	MODE_SUBNEG_IAC,
};

typedef struct sClient
{
	enum eTelnetMode	Mode;
	 int	Socket;
	 int	pty;
} tClient;

// === PROTOTYPES ===
void	EventLoop(void);
void	Server_NewClient(int FD);
void	HandleServerBoundData(tClient *Client);
void	HandleClientBoundData(tClient *Client);
void	HandleOptionRequest(tClient *Client, int Option, bool Value, bool IsRequest);

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
			FD_SET_MAX(&fds, gaClients[i].Socket, &maxfd);
			FD_SET_MAX(&fds, gaClients[i].pty,  &maxfd);
		}
		
		// Select!
		int rv = _SysSelect( maxfd+1, &fds, NULL, NULL, NULL, 0 );
		_SysDebug("Select return %i", rv);
		
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
	
	// Create PTY
	// TODO: Use PTYs
	clt->pty = _SysOpen("/Devices/pts/ptmx", OPENFLAG_READ|OPENFLAG_WRITE);
	if( clt->pty < 0 ) {
		perror("Unable to create/open PTY");
		_SysDebug("Unable to create/open PTY: %s", strerror(errno));
		_SysClose(clt->Socket);
		clt->Socket = 0;
		return ;
	}
	// - Initialise
	{
		_SysIOCtl(clt->pty, PTY_IOCTL_SETID, "telnetd#");
		struct ptymode	mode = {.InputMode = 0, .OutputMode=0};
		struct ptydims	dims = {.W = 80, .H = 25};
		_SysIOCtl(clt->pty, PTY_IOCTL_SETMODE, &mode);
		_SysIOCtl(clt->pty, PTY_IOCTL_SETDIMS, &dims);
	}
	
	// TODO: Arguments and envp
	{
		char pty_path[] = "/Devices/pts/telnetd000";
		_SysIOCtl(clt->pty, PTY_IOCTL_GETID, pty_path+13);
		 int	clientfd = _SysOpen(pty_path, OPENFLAG_READ|OPENFLAG_WRITE);
		if(clientfd < 0) {
			perror("Unable to open login PTY");
			_SysClose(clt->Socket);
			_SysClose(clt->pty);
			clt->Socket = 0;
			return ;
		}
		_SysDebug("Using client PTY %s", pty_path);
		int fds[3] = {clientfd, clientfd, clientfd};
		const char	*argv[] = {"login", NULL};
		_SysSpawn("/Acess/SBin/login", argv, argv, 3, fds, NULL);
		_SysClose(clientfd);
	}
}

void HandleServerBoundData(tClient *Client)
{
	uint8_t	buf[BUFSIZ];
	size_t	len;

	_SysDebug("Client %p", Client);	
	len = _SysRead(Client->Socket, buf, BUFSIZ);
	_SysDebug("%i bytes for %p", len, Client);
	if( len == 0 )	return ;
	if( len == -1 ) {
		return ;
	}
	// handle options
	// int	last_flush = 0;
	for( int i = 0; i < len; i ++ )
	{
		switch(Client->Mode)
		{
		case MODE_IAC:
			Client->Mode = MODE_DATA;
			switch(buf[i])
			{
			case 240:	// End of subnegotiation parameters
				_SysDebug("End Subnegotiation");
				break;
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
				break;
			case 250:	// Subnegotation
				_SysDebug("Subnegotiation");
				// TODO: Get option ID, and then cache until 'END SB' (240)
				Client->Mode = MODE_SUBNEG_OPTION;
				break;
			case 251:	// WILL
				Client->Mode = MODE_WILL;
				break;
			case 252:	// WONT
				Client->Mode = MODE_WONT;
				break;
			case 253:	// DO
				Client->Mode = MODE_DO;
				break;
			case 254:	// DONT
				Client->Mode = MODE_DONT;
				break;
			case 255:	// Literal 255
				_SysWrite(Client->pty, buf+i, 1);
				break;
			}
			break;
		case MODE_WILL:
			_SysDebug("Option %i WILL", buf[i]);
			HandleOptionRequest(Client, buf[i], true, false);
			Client->Mode = MODE_DATA;
			break;
		case MODE_WONT:
			_SysDebug("Option %i WONT", buf[i]);
			HandleOptionRequest(Client, buf[i], false, false);
			Client->Mode = MODE_DATA;
			break;
		case MODE_DO:
			_SysDebug("Option %i DO", buf[i]);
			HandleOptionRequest(Client, buf[i], true, true);
			Client->Mode = MODE_DATA;
			break;
		case MODE_DONT:
			_SysDebug("Option %i DONT", buf[i]);
			HandleOptionRequest(Client, buf[i], false, true);
			Client->Mode = MODE_DATA;
			break;
		case MODE_SUBNEG_OPTION:
			_SysDebug("Option %i subnegotation", buf[i]);
			Client->Mode = MODE_SUBNEG_DATA;
			break;
		case MODE_SUBNEG_IAC:
			switch(buf[i])
			{
			case 240:	// End subnegotation
				// TODO: Handle subnegotation data
				Client->Mode = MODE_DATA;
				break;
			case 255:
				// TODO: Literal 255
				Client->Mode = MODE_SUBNEG_DATA;
				break;
			default:
				_SysDebug("Unexpected %i in SUBNEG IAC", buf[i]);
				Client->Mode = MODE_SUBNEG_DATA;
				break;
			}
		case MODE_SUBNEG_DATA:
			if( buf[i] == 255 )
				Client->Mode = MODE_SUBNEG_IAC;
			else
				;//_SysWrite(Client->pty, buf+i, 1);
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

void HandleOptionRequest(tClient *Client, int Option, bool Value, bool IsRequest)
{
	switch(Option)
	{
	default:
		_SysDebug("Unknown option %i", Option);
		break;
	}
}


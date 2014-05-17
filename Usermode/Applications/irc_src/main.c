/*
 * Acess2 IRC Client
 */
#include <acess/sys.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <net.h>
#include <stdarg.h>

#include "common.h"
#include "input.h"
#include "window.h"
#include "server.h"

// === TYPES ===

// === PROTOTYPES ===
 int	main(int argc, const char *argv[], const char *envp[]);
 int	MainLoop(void);
 int	ParseArguments(int argc, const char *argv[]);
// --- 
void	Redraw_Screen(void);
// --- Helpers
void	Exit(const char *Reason);
 int	writef(int FD, const char *Format, ...);
 int	OpenTCP(const char *AddressString, short PortNumber);
char	*GetValue(char *Str, int *Ofs);

// === GLOBALS ===
const char	*gsExitReason = "No reason [BUG]";

// ==== CODE ====
void ExitHandler(void)
{
	printf("\x1B[?1047l");
	printf("Quit: %s\n", gsExitReason);
}

void Exit(const char *Reason)
{
	gsExitReason = (Reason ? Reason : "User Requested");
	exit( (Reason ? 1 : 0) );
}

int main(int argc, const char *argv[], const char *envp[])
{
	 int	tmp;
	
	// Parse Command line
	if( (tmp = ParseArguments(argc, argv)) )	return tmp;
	
	atexit(ExitHandler);
	
	ACurses_Init();
	
	printf("\x1B[?1047h");
	printf("\x1B[%i;%ir", 1, giTerminal_Height-1);
	
	SetCursorPos(giTerminal_Height-1, 1);
	printf("[(status)] ");
	
	// HACK: Static server entry
	// UCC (University [of Western Australia] Computer Club) IRC Server
	tServer	*starting_server = Server_Connect( "UCC", "130.95.13.18", 6667 );
	// Freenode (#osdev)
//	gWindow_Status.Server = Server_Connect( "Freenode", "89.16.176.16", 6667 );
	// Local servers
//	gWindow_Status.Server = Server_Connect( "VMHost", "10.0.2.2", 6667 );
//	gWindow_Status.Server = Server_Connect( "BitlBee", "192.168.1.39", 6667 );
	
	if( !starting_server )
		return -1;
	
	Windows_SetStatusServer(starting_server);
	
	MainLoop();

	Servers_CloseAll("Client closing");
	
	return 0;
}

int MainLoop(void)
{
	SetCursorPos(giTerminal_Height-1, 1);
	printf("[(status)] ");
	fflush(stdout);
	
	for( ;; )
	{
		fd_set	readfds, errorfds;
		 int	nfds = 1;
		
		FD_ZERO(&readfds);
		FD_ZERO(&errorfds);
		
		Input_FillSelect(&nfds, &readfds);
		Servers_FillSelect(&nfds, &readfds, &errorfds);
		
		int rv = _SysSelect(nfds, &readfds, 0, &errorfds, NULL, 0);
		if( rv < 0 )	break;
		
		// user input
		Input_HandleSelect(nfds, &readfds);
		
		// Server response
		Servers_HandleSelect(nfds, &readfds, &errorfds);
	}
	return 0;
}

/**
 * \todo Actually implement correctly :)
 */
int ParseArguments(int argc, const char *argv[])
{
	return 0;
}

void Redraw_Screen(void)
{
	printf("\x1B[2J");	// Clear screen
	printf("\x1B[0;0H");	// Reset cursor

	Windows_RepaintCurrent();
}

/**
 * \brief Write a formatted string to a file descriptor
 * 
 */
int writef(int FD, const char *Format, ...)
{
	va_list	args;
	 int	len;
	
	va_start(args, Format);
	len = vsnprintf(NULL, 0, Format, args);
	va_end(args);
	
	{
		char	buf[len+1];
		va_start(args, Format);
		vsnprintf(buf, len+1, Format, args);
		va_end(args);
		
		return _SysWrite(FD, buf, len);
	}
}

/**
 * \brief Initialise a TCP connection to \a AddressString on port \a PortNumber
 */
int OpenTCP(const char *AddressString, short PortNumber)
{
	 int	fd, addrType;
	char	addrBuffer[8];
	
	// Parse IP Address
	addrType = Net_ParseAddress(AddressString, addrBuffer);
	if( addrType == 0 ) {
		fprintf(stderr, "Unable to parse '%s' as an IP address\n", AddressString);
		_SysDebug("Unable to parse '%s' as an IP address\n", AddressString);
		return -1;
	}
	
	// Finds the interface for the destination address
	fd = Net_OpenSocket(addrType, addrBuffer, "tcpc");
	if( fd == -1 ) {
		fprintf(stderr, "Unable to open TCP Client to '%s'\n", AddressString);
		_SysDebug("Unable to open TCP client to '%s'\n", AddressString);
		return -1;
	}
	
	// Set remote port and address
//	printf("Setting port and remote address\n");
	_SysIOCtl(fd, 5, &PortNumber);
	_SysIOCtl(fd, 6, addrBuffer);
	
	// Connect
//	printf("Initiating connection\n");
	if( _SysIOCtl(fd, 7, NULL) == 0 ) {
		// Shouldn't happen :(
		fprintf(stderr, "Unable to start connection\n");
		return -1;
	}
	
	// Return descriptor
	return fd;
}

/**
 * \brief Read a space-separated value from a string
 */
char *GetValue(char *Src, int *Ofs)
{
	 int	pos = *Ofs;
	char	*ret = Src + pos;
	char	*end;
	
	if( !Src )	return NULL;
	
	while( *ret == ' ' )	ret ++;
	
	end = strchr(ret, ' ');
	if( end ) {
		*end = '\0';
	}
	else {
		end = ret + strlen(ret) - 1;
	}
	
	end ++ ;
	while( *ret == ' ' )	end ++;
	*Ofs = end - Src;
	
	return ret;
}


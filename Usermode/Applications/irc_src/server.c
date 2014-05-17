/*
 */
#include <stdlib.h>
#include <string.h>
#include "server.h"
#include "window.h"
#include <acess/sys.h>
#include <ctype.h>	// isdigit
#include <stdio.h>	// vsnprintf

// === PROTOTYPES ===
tServer	*Server_Connect(const char *Name, const char *AddressString, short PortNumber);
 int	Server_HandleIncoming(tServer *Server);
void	ParseServerLine(tServer *Server, char *Line);

// === GLOBALS ===
const char	*gsUsername = "user";
const char	*gsHostname = "acess";
const char	*gsRealName = "Acess2 IRC Client";
const char	*gsNickname = "acess";
tServer	*gpServers;

// === CODE ===
void Servers_FillSelect(int *nfds, fd_set *rfds, fd_set *efds)
{
	for( tServer *srv = gpServers; srv; srv = srv->Next )
	{
		FD_SET(srv->FD, rfds);
		FD_SET(srv->FD, efds);
		if( srv->FD >= *nfds )
			*nfds = srv->FD+1;
	}
}

void Servers_HandleSelect(int nfds, const fd_set *rfds, const fd_set *efds)
{
	for( tServer *srv = gpServers; srv; srv = srv->Next )
	{
		if(FD_ISSET(srv->FD, rfds))
		{
			 int	rv = Server_HandleIncoming(srv);
			if(rv)
			{
				// Oops, error
				_SysDebug("ProcessIncoming failed on FD%i (Server %p %s)",
					srv->FD, srv, srv->Name);
				Exit("Processing error");
			}
		}
		
		if(FD_ISSET(srv->FD, efds))
		{
			_SysDebug("Error on FD%i (Server %p %s)",
				srv->FD, srv, srv->Name);
			Exit("Socket error");
		}
	}
}

void Servers_CloseAll(const char *QuitMessage)
{
	while( gpServers )
	{
		tServer *srv = gpServers;
		gpServers = srv->Next;
	
		Server_SendCommand(srv, "QUIT :%s", QuitMessage);
		_SysClose(srv->FD);
		free(srv);
	}
}

/**
 * \brief Connect to a server
 */
tServer *Server_Connect(const char *Name, const char *AddressString, short PortNumber)
{
	tServer	*ret;
	
	ret = calloc(1, sizeof(tServer) + strlen(Name) + 1);
	
	strcpy(ret->Name, Name);
	
	// Connect to the remove server
	ret->FD = OpenTCP( AddressString, PortNumber );
	if( ret->FD == -1 ) {
		free(ret);
		Window_AppendMessage(WINDOW_STATUS, MSG_CLASS_CLIENT, Name, "Unable to create socket");
		return NULL;
	}
	
	ret->Nick = strdup(gsNickname);
	
	// Append to open list
	ret->Next = gpServers;
	gpServers = ret;
	
	// Read some initial data
	Window_AppendMessage(WINDOW_STATUS, MSG_CLASS_CLIENT, Name, "Unable to create socket");
	Server_HandleIncoming(ret);
	
	// Identify
	Server_SendCommand(ret, "USER %s %s %s : %s", gsUsername, gsHostname, AddressString, gsRealName);
	Server_SendCommand(ret, "NICK %s", ret->Nick);
	Window_AppendMessage(WINDOW_STATUS, MSG_CLASS_CLIENT, Name, "Identified");
	
	return ret;
}

const char *Server_GetNick(const tServer *Server)
{
	return Server->Nick;
}

void Server_SendCommand(tServer *Server, const char *Format, ...)
{
	va_list	args;
	 int	len;
	
	va_start(args, Format);
	len = vsnprintf(NULL, 0, Format, args);
	va_end(args);
	
	char	buf[len+1];
	va_start(args, Format);
	vsnprintf(buf, len+1, Format, args);
	va_end(args);
	
	_SysWrite(Server->FD, buf, len);
	_SysWrite(Server->FD, "\n", 1);
}

void Cmd_PRIVMSG(tServer *Server, const char *Dest, const char *Src, const char *Message)
{
	tWindow *win;
	if( strcmp(Dest, Server->Nick) == 0 ) {
		win = Windows_GetByName(Server, Src);
		if(!win)
			win = Window_Create(Server, Src);
	}
	else {
		win = Windows_GetByName(Server, Dest);
		if(!win)
			win = Window_Create(Server, Dest);
	}
	Window_AppendMessage(win, MSG_CLASS_MESSAGE, Src, "%s", Message);
}

/**
 * \brief Process incoming lines from the server
 */
int Server_HandleIncoming(tServer *Server)
{	
	char	*ptr, *newline;
	 int	len;
	
	// While there is data in the buffer, read it into user memory and 
	// process it line by line
	// ioctl#8 on a TCP client gets the number of bytes in the recieve buffer
	// - Used to avoid blocking
	#if NON_BLOCK_READ
	while( (len = _SysIOCtl(Server->FD, 8, NULL)) > 0 )
	{
	#endif
		// Read data
		len = _SysRead(Server->FD, &Server->InBuf[Server->ReadPos], sizeof(Server->InBuf) - Server->ReadPos);
		if( len == -1 ) {
			return -1;
		}
		Server->InBuf[Server->ReadPos + len] = '\0';
		
		// Break into lines
		ptr = Server->InBuf;
		while( (newline = strchr(ptr, '\n')) )
		{
			*newline = '\0';
			if( newline[-1] == '\r' )	newline[-1] = '\0';
			ParseServerLine(Server, ptr);
			ptr = newline + 1;
		}
		
		// Handle incomplete lines
		if( ptr - Server->InBuf < len + Server->ReadPos ) {
			// Update the read position
			// InBuf ReadPos    ptr          ReadPos+len
			// | old | new used | new unused |
			Server->ReadPos = len + Server->ReadPos - (ptr - Server->InBuf);
			// Copy stuff back (moving "new unused" to the start of the buffer)
			memcpy(Server->InBuf, ptr, Server->ReadPos);
		}
		else {
			Server->ReadPos = 0;
		}
	#if NON_BLOCK_READ
	}
	#endif
	
	return 0;
}

void ParseServerLine_Numeric(tServer *Server, const char *ident, int Num, char *Line)
{
	 int	pos = 0;
	const char *message;
	const char *user = GetValue(Line, &pos);
	const char	*timestamp;
	
	if( Line[pos] == ':' ) {
		message = Line + pos + 1;
	}
	else {
		message = GetValue(Line, &pos);
	}
	
	switch(Num)
	{
	case 332:	// Topic
		user = message;	// Channel
		message = Line + pos + 1;	// Topic
		Window_AppendMsg_Topic( Windows_GetByNameOrCreate(Server, user), message );
		break;
	case 333:	// Topic set by
		user = message;	// Channel
		message = GetValue(Line, &pos);	// User
		timestamp = GetValue(Line, &pos);	// Timestamp
		Window_AppendMsg_TopicTime( Windows_GetByNameOrCreate(Server, user), message, timestamp );
		break;
	case 353:	// /NAMES list
		// <user> = <channel> :list
		// '=' was eaten in and set to message
		user = GetValue(Line, &pos);	// Actually channel
		message = Line + pos + 1;	// List
		// TODO: parse and store
		Window_AppendMessage( Windows_GetByNameOrCreate(Server, user), MSG_CLASS_CLIENT, "NAMES", message );
		break;
	case 366:	// end of /NAMES list
		// <user> <channel> :msg
		// - Ignored
		break;
	case 372:	// MOTD Data
	case 375:	// MOTD Start
	case 376:	// MOTD End
		
	default:
		//printf("[%s] %i %s\n", Server->Name, num, message);
		Window_AppendMessage( WINDOW_STATUS, MSG_CLASS_WALL, Server->Name, "Unknown %i %s", Num, message);
		break;
	}
}

void ParseServerLine_String(tServer *Server, const char *ident, const char *cmd, char *Line)
{
	 int	pos = 0;
	_SysDebug("ident=%s,cmd=%s,Line=%s", ident, cmd, Line);
	if( strcmp(cmd, "NOTICE") == 0 )
	{
		const char *class = GetValue(Line, &pos);
		_SysDebug("NOTICE class='%s'", class);
		
		const char *message = (Line[pos] == ':') ? Line + pos + 1 : GetValue(Line, &pos);
		
		//printf("[%s] NOTICE %s: %s\n", Server->Name, ident, message);
		char *ident_bang = strchr(ident, '!');
		if( ident_bang ) {
			*ident_bang = '\0';
		}
		// TODO: Colour codes
		Window_AppendMessage( WINDOW_STATUS, MSG_CLASS_WALL, Server->Name, "%s %s", ident, message);
	}
	else if( strcmp(cmd, "PRIVMSG") == 0 )
	{
		const char *dest = GetValue(Line, &pos);
		const char *message = (Line[pos] == ':') ? Line + pos + 1 : GetValue(Line, &pos);

		char *ident_bang = strchr(ident, '!');
		if( ident_bang ) {
			*ident_bang = '\0';
		}
		Cmd_PRIVMSG(Server, dest, ident, message);
	}
	else if( strcmp(cmd, "JOIN" ) == 0 )
	{
		const char	*channel = Line + pos + 1;
		
		Window_AppendMsg_Join( Windows_GetByNameOrCreate(Server, channel), ident );
	}
	else if( strcmp(cmd, "PART" ) == 0 )
	{
		const char	*channel = Line + pos + 1;
		
		Window_AppendMsg_Part( Windows_GetByNameOrCreate(Server, channel), ident, "" );
	}
	else
	{
		Window_AppendMessage( WINDOW_STATUS, MSG_CLASS_BARE, Server->Name, "Unknown command '%s' %s", cmd, Line);
	}
}

/**
 */
void ParseServerLine(tServer *Server, char *Line)
{
	 int	pos = 0;

	_SysDebug("[%s] %s", Server->Name, Line);	
	
	// Message?
	if( *Line == ':' )
	{
		pos ++;
		const char *ident = GetValue(Line, &pos);	// Ident (user or server)
		const char *cmd = GetValue(Line, &pos);
		
		// Numeric command
		if( isdigit(cmd[0]) && isdigit(cmd[1]) && isdigit(cmd[2]) )
		{
			 int	num;
			num  = (cmd[0] - '0') * 100;
			num += (cmd[1] - '0') * 10;
			num += (cmd[2] - '0') * 1;

			ParseServerLine_Numeric(Server, ident, num, Line+pos);
		}
		else
		{
			ParseServerLine_String(Server, ident, cmd, Line+pos);
		}
	}
	else {
		const char *cmd = GetValue(Line, &pos);
		
		if( strcmp(cmd, "PING") == 0 ) {
			Server_SendCommand(Server, "PONG %s", Line+pos);
		}
		else {
			// Command to client
			Window_AppendMessage(WINDOW_STATUS, MSG_CLASS_CLIENT, Server->Name, "UNK Client Command: %s", Line);
		}
	}
}

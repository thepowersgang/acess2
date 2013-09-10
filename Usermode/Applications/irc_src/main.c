/*
 * Acess2 IRC Client
 */
#include <acess/sys.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <net.h>
#include <readline.h>
#include <acess/devices/pty.h>

// === TYPES ===
typedef struct sServer {
	struct sServer	*Next;
	 int	FD;
	char	InBuf[BUFSIZ+1];
	 int	ReadPos;
	char	Name[];
} tServer;

typedef struct sMessage
{
	struct sMessage	*Next;
	time_t	Timestamp;
	tServer	*Server;
	 int	Type;
	char	*Source;	// Pointer into `Data`
	char	Data[];
}	tMessage;

typedef struct sWindow
{
	struct sWindow	*Next;
	tMessage	*Messages;
	tServer	*Server;	//!< Canoical server (can be NULL)
	 int	ActivityLevel;
	char	Name[];	// Channel name / remote user
}	tWindow;

enum eMessageTypes
{
	MSG_TYPE_NULL,
	MSG_TYPE_SERVER,	// Server message
	
	MSG_TYPE_NOTICE,	// NOTICE command
	MSG_TYPE_JOIN,	// JOIN command
	MSG_TYPE_PART,	// PART command
	MSG_TYPE_QUIT,	// QUIT command
	
	MSG_TYPE_STANDARD,	// Standard line
	MSG_TYPE_ACTION,	// /me
	
	MSG_TYPE_UNK
};

// === PROTOTYPES ===
 int	ParseArguments(int argc, const char *argv[]);
 int	ParseUserCommand(char *String);
// --- 
tServer	*Server_Connect(const char *Name, const char *AddressString, short PortNumber);
tMessage	*Message_AppendF(tServer *Server, int Type, const char *Source, const char *Dest, const char *Message, ...);
tMessage	*Message_Append(tServer *Server, int Type, const char *Source, const char *Dest, const char *Message);
tWindow	*Window_Create(tServer *Server, const char *Name);
void	Redraw_Screen(void);

 int	ProcessIncoming(tServer *Server);
// --- Helpers
void	SetCursorPos(int Row, int Col);
 int	writef(int FD, const char *Format, ...);
 int	OpenTCP(const char *AddressString, short PortNumber);
char	*GetValue(char *Str, int *Ofs);
static inline int	isdigit(int ch);

// === GLOBALS ===
char	*gsUsername = "user";
char	*gsHostname = "acess";
char	*gsRealName = "Acess2 IRC Client";
char	*gsNickname = "acess";
tServer	*gpServers;
tWindow	gWindow_Status = {
	NULL, NULL, NULL,	// No next, empty list, no server
	0, {""}	// No activity, empty name (rendered as status)
};
tWindow	*gpWindows = &gWindow_Status;
tWindow	*gpCurrentWindow = &gWindow_Status;
 int	giTerminal_Width = 80;
 int	giTerminal_Height = 25;

// ==== CODE ====
void ExitHandler(void)
{
	printf("\x1B[?1047l");
	printf("Quit\n");
}

int main(int argc, const char *argv[], const char *envp[])
{
	 int	tmp;
	tReadline	*readline_info;
	
	// Parse Command line
	if( (tmp = ParseArguments(argc, argv)) )	return tmp;
	
	atexit(ExitHandler);
	
	if( _SysIOCtl(1, DRV_IOCTL_TYPE, NULL) != DRV_TYPE_TERMINAL ) {
		fprintf(stderr, "note: assuming 80x25, can't get terminal dimensions\n");
		giTerminal_Width = 80;
		giTerminal_Height = 25;
	}
	else {
		struct ptydims	dims;
		_SysIOCtl(1, PTY_IOCTL_GETDIMS, &dims);
		giTerminal_Width = dims.W;
		giTerminal_Height = dims.H;
	}
	
	printf("\x1B[?1047h");
	printf("\x1B[%i;%ir", 0, giTerminal_Height-1);
	
	SetCursorPos(giTerminal_Height-1, 0);
	printf("[(status)] ");
	
	// HACK: Static server entry
	// UCC (University [of Western Australia] Computer Club) IRC Server
	gWindow_Status.Server = Server_Connect( "UCC", "130.95.13.18", 6667 );
	// Freenode (#osdev)
//	gWindow_Status.Server = Server_Connect( "Freenode", "89.16.176.16", 6667 );
//	gWindow_Status.Server = Server_Connect( "Host", "10.0.2.2", 6667 );
	// Local server
//	gWindow_Status.Server = Server_Connect( "BitlBee", "192.168.1.39", 6667 );
	
	if( !gWindow_Status.Server )
		return -1;
	
	SetCursorPos(giTerminal_Height-1, 0);
	printf("[(status)] ");
	fflush(stdout);
	readline_info = Readline_Init(1);
	
	for( ;; )
	{
		fd_set	readfds, errorfds;
		 int	rv, maxFD = 0;
		tServer	*srv;
		
		FD_ZERO(&readfds);
		FD_ZERO(&errorfds);
		FD_SET(0, &readfds);	// stdin
		
		// Fill server FDs in fd_set
		for( srv = gpServers; srv; srv = srv->Next )
		{
			FD_SET(srv->FD, &readfds);
			FD_SET(srv->FD, &errorfds);
			if( srv->FD > maxFD )
				maxFD = srv->FD;
		}
		
		rv = _SysSelect(maxFD+1, &readfds, 0, &errorfds, NULL, 0);
		if( rv == -1 )	break;
		
		if(FD_ISSET(0, &readfds))
		{
			// User input
			char	*cmd = Readline_NonBlock(readline_info);
			if( cmd )
			{
				if( cmd[0] )
				{
					ParseUserCommand(cmd);
				}
				free(cmd);
				// Prompt
				SetCursorPos(giTerminal_Height-1, 0);
				printf("\x1B[K");	// Clear line
				if( gpCurrentWindow->Name[0] )
					printf("[%s:%s] ", gpCurrentWindow->Server->Name, gpCurrentWindow->Name);
				else
					printf("[(status)] ");
				fflush(stdout);
			}
		}
		
		// Server response
		for( srv = gpServers; srv; srv = srv->Next )
		{
			if(FD_ISSET(srv->FD, &readfds))
			{
				if( ProcessIncoming(srv) != 0 ) {
					// Oops, error
					break;
				}
			}
			
			if(FD_ISSET(srv->FD, &errorfds))
			{
				break;
			}
		}
		
		// Oops, an error
		if( srv )	break;
	}
	
	{
		tServer *srv;
		for( srv = gpServers; srv; srv = srv->Next )
			_SysClose(srv->FD);
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


void Cmd_join(char *ArgString)
{
	 int	pos=0;
	char	*channel_name = GetValue(ArgString, &pos);
	
	if( gpCurrentWindow->Server )
	{
		writef(gpCurrentWindow->Server->FD, "JOIN :%s\n", channel_name);
	}
}

void Cmd_quit(char *ArgString)
{
	const char *quit_message = ArgString;
	if( quit_message == NULL || quit_message[0] == '\0' )
		quit_message = "/quit - Acess2 IRC Client";
	
	for( tServer *srv = gpServers; srv; srv = srv->Next )
	{
		writef(srv->FD, "QUIT :%s\n", quit_message);
	}
	
	exit(0);
}

void Cmd_window(char *ArgString)
{
	 int	pos = 0;
	char	*window_id = GetValue(ArgString, &pos);
	 int	window_num = atoi(window_id);
	
	if( window_num > 0 )
	{
		tWindow	*win;
		window_num --;	// Move to base 0
		// Get `window_num`th window
		for( win = gpWindows; win && window_num--; win = win->Next );
		if( win ) {
			gpCurrentWindow = win;
			Redraw_Screen();
		}
		// Otherwise, silently ignore
	}
	else
	{
		window_num = 1;
		for( tWindow *win = gpWindows; win; win = win->Next, window_num ++ )
		{
			if( win->Name[0] ) {
				Message_AppendF(NULL, MSG_TYPE_SERVER, "client", "",
					"%i: %s/%s", window_num, win->Server->Name, win->Name);
			}
			else {
				Message_AppendF(NULL, MSG_TYPE_SERVER, "client", "",
					"%i: (status)", window_num);
			}
		}
	}
}

const struct {
	const char *Name;
	void	(*Fcn)(char *ArgString);
} caCommands[] = {
	{"join", Cmd_join},
	{"quit", Cmd_quit},
	{"window", Cmd_window},
	{"win",    Cmd_window},
	{"w",      Cmd_window},
};
const int ciNumCommands = sizeof(caCommands)/sizeof(caCommands[0]);

/**
 * \brief Handle a line from the prompt
 */
int ParseUserCommand(char *String)
{
	if( String[0] == '/' )
	{
		char	*command;
		 int	pos = 0;
		
		command = GetValue(String, &pos)+1;

		// TODO: Prefix matches
		 int	cmdIdx = -1;
		for( int i = 0; i < ciNumCommands; i ++ )
		{
			if( strcmp(command, caCommands[i].Name) == 0 ) {
				cmdIdx = i;
				break;
			}
		}
		if( cmdIdx != -1 ) {
			caCommands[cmdIdx].Fcn(String+pos);
		}
		else
		{
			Message_AppendF(NULL, MSG_TYPE_SERVER, "client", "", "Unknown command %s", command);
		}
	}
	else
	{
		// Message
		// - Only send if server is valid and window name is non-empty
		if( gpCurrentWindow->Server && gpCurrentWindow->Name[0] )
		{
			Message_Append(gpCurrentWindow->Server, MSG_TYPE_STANDARD,
				gsNickname, gpCurrentWindow->Name, String);
			writef(gpCurrentWindow->Server->FD,
				"PRIVMSG %s :%s\n", gpCurrentWindow->Name,
				String
				);
		}
	}
	
	return 0;
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
		fprintf(stderr, "%s: Unable to create socket\n", Name);
		return NULL;
	}
	
	// Append to open list
	ret->Next = gpServers;
	gpServers = ret;
	
	// Read some initial data
	Message_Append(NULL, MSG_TYPE_SERVER, Name, "", "Connection opened");
	ProcessIncoming(ret);
	
	// Identify
	writef(ret->FD, "USER %s %s %s : %s\n", gsUsername, gsHostname, AddressString, gsRealName);
	writef(ret->FD, "NICK %s\n", gsNickname);
	Message_Append(NULL, MSG_TYPE_SERVER, Name, "", "Identified");
	//printf("%s: Identified\n", Name);
	
	return ret;
}

tMessage *Message_AppendF(tServer *Server, int Type, const char *Source, const char *Dest, const char *Message, ...)
{
	va_list	args;
	 int	len;
	va_start(args, Message);
	len = vsnprintf(NULL, 0, Message, args);
	{
		char	buf[len+1];
		vsnprintf(buf, len+1, Message, args);
		return Message_Append(Server, Type, Source, Dest, buf);
	}
}

tMessage *Message_Append(tServer *Server, int Type, const char *Source, const char *Dest, const char *Message)
{
	tWindow	*win = NULL;
	 int	msgLen = strlen(Message);
	
	// Server==NULL indicates an internal message
	if( Server == NULL || Source[0] == '\0' )
	{
		win = &gWindow_Status;
	}
	// Determine if it's a channel or PM
	else if( Dest[0] == '#' || Dest[0] == '&' )	// TODO: Better determining here
	{
		for(win = gpWindows; win; win = win->Next)
		{
			if( win->Server == Server && strcmp(win->Name, Dest) == 0 )
			{
				break;
			}
		}
		if( !win ) {
			win = Window_Create(Server, Dest);
		}
	}
	#if 0
	else if( strcmp(Dest, Server->Nick) != 0 )
	{
		// Umm... message for someone who isn't us?
		win = &gWindow_Status;	// Stick it in the status window, just in case
	}
	#endif
	// Server message?
	else if( strchr(Source, '.') )	// TODO: And again, less hack please
	{
		#if 1
		for(win = gpWindows; win; win = win->Next)
		{
			if( win->Server == Server && strcmp(win->Name, Source) == 0 )
			{
				break;
			}
		}
		#endif
		if( !win ) {
			win = &gWindow_Status;
		}
		
	}
	// Private message
	else
	{
		for(win = gpWindows; win; win = win->Next)
		{
			if( win->Server == Server && strcmp(win->Name, Source) == 0 )
			{
				break;
			}
		}
		if( !win ) {
			win = Window_Create(Server, Dest);
		}
	}

	// Create message cache	
	_SysDebug("Win (%s) msg: <%s> %s", win->Name, Source, Message);
	tMessage	*ret;
	ret = malloc( sizeof(tMessage) + msgLen + 1 + strlen(Source) + 1 );
	ret->Source = ret->Data + msgLen + 1;
	strcpy(ret->Source, Source);
	strcpy(ret->Data, Message);
	ret->Type = Type;
	ret->Server = Server;
	
	// Append to window message list
	ret->Next = win->Messages;
	win->Messages = ret;
	
	// Print now if current window
	if( win == gpCurrentWindow )
	{
		printf("\x1b[s");
		printf("\x1b""D");	// Scroll down 1 (free space below)
		SetCursorPos(giTerminal_Height-2, 0);
		 int	prefixlen = strlen(Source) + 3;
		 int	avail = giTerminal_Width - prefixlen;
		 int	msglen = strlen(Message);
		printf("[%s] %.*s", Source, avail, Message);
		while( msglen > avail ) {
			msglen -= avail;
			Message += avail;
			printf("\x1b""D");
			SetCursorPos(giTerminal_Height-2, prefixlen);
			printf("%.*s\n", avail, Message);
		}
		printf("\x1b[u");
	}
	
	return ret;
}

tWindow *Window_Create(tServer *Server, const char *Name)
{
	tWindow	*ret, *prev = NULL;
	 int	num = 1;
	
	// Get the end of the list
	// TODO: Cache this instead
	for( ret = gpCurrentWindow; ret; prev = ret, ret = ret->Next )
		num ++;
	
	ret = malloc(sizeof(tWindow) + strlen(Name) + 1);
	ret->Messages = NULL;
	ret->Server = Server;
	ret->ActivityLevel = 1;
	strcpy(ret->Name, Name);
	
	if( prev ) {
		ret->Next = prev->Next;
		prev->Next = ret;
	}
	else {	// Shouldn't happen really
		ret->Next = gpWindows;
		gpWindows = ret;
	}
	
//	printf("Win %i %s:%s created\n", num, Server->Name, Name);
	
	return ret;
}

void Redraw_Screen(void)
{
	 int	y = 0;
	tMessage	*msg;

	printf("\x1B[2J");	// Clear screen
	printf("\x1B[0;0H");	// Reset cursor

	msg = gpCurrentWindow->Messages;
	
	// TODO: Title bar?

	// Note: This renders from the bottom up
	for( y = giTerminal_Height - 1; y -- && msg; msg = msg->Next)
	{
		 int	msglen = strlen(msg->Data);
		 int	prefix_len = 3 + strlen(msg->Source);
		 int	line_avail = giTerminal_Width - prefix_len;
		 int	i = 0, done = 0;
		
		y -= msglen / line_avail;	// Extra lines (y-- above handles the 1 line case)
		SetCursorPos(y, 0);
		printf("[%s] ", msg->Source);
		
		while(done < msglen) {
			done += printf("%.*s", line_avail, msg->Data+done);
			i ++;
			SetCursorPos(y+i, prefix_len);
		}
	}

	// Bottom line is rendered by the prompt
}

void Cmd_PRIVMSG(tServer *Server, const char *Dest, const char *Src, const char *Message)
{
	Message_Append(Server, MSG_TYPE_STANDARD, Dest, Src, Message);
	//printf("<%s:%s:%s> %s\n", Server->Name, Dest, Src, Message);
}

/**
 */
void ParseServerLine(tServer *Server, char *Line)
{
	 int	pos = 0;
	char	*ident, *cmd;

//	_SysDebug("[%s] %s", Server->Name, Line);	
	
	// Message?
	if( *Line == ':' )
	{
		pos ++;
		ident = GetValue(Line, &pos);	// Ident (user or server)
		cmd = GetValue(Line, &pos);
		
		// Numeric command
		if( isdigit(cmd[0]) && isdigit(cmd[1]) && isdigit(cmd[2]) )
		{
			char	*user, *message;
			 int	num;
			num  = (cmd[0] - '0') * 100;
			num += (cmd[1] - '0') * 10;
			num += (cmd[2] - '0') * 1;
			
			user = GetValue(Line, &pos);
			
			if( Line[pos] == ':' ) {
				message = Line + pos + 1;
			}
			else {
				message = GetValue(Line, &pos);
			}
			
			switch(num)
			{
			case 332:	// Topic
				user = message;	// Channel
				message = Line + pos + 1;	// Topic
				Message_AppendF(Server, MSG_TYPE_SERVER, user, user, "Topic: %s", message);
				break;
			case 333:	// Topic set by
				user = message;	// Channel
				message = GetValue(Line, &pos);	// User
				GetValue(Line, &pos);	// Timestamp
				Message_AppendF(Server, MSG_TYPE_SERVER, user, user, "Set by %s", message);
				break;
			case 353:	// /NAMES list
				// <user> = <channel> :list
				// '=' was eaten in and set to message
				user = GetValue(Line, &pos);	// Actually channel
				message = Line + pos + 1;	// List
				Message_AppendF(Server, MSG_TYPE_SERVER, user, user, "Names: %s", message);
				break;
			case 366:	// end of /NAMES list
				// <user> <channel> :msg
				user = message;
				message = Line + pos + 1;
				Message_Append(Server, MSG_TYPE_SERVER, user, user, message);
				break;
			case 372:	// MOTD Data
			case 375:	// MOTD Start
			case 376:	// MOTD End
				
			default:
				//printf("[%s] %i %s\n", Server->Name, num, message);
				Message_Append(Server, MSG_TYPE_SERVER, ident, user, message);
				break;
			}
		}
		else if( strcmp(cmd, "NOTICE") == 0 )
		{
			char	*class, *message;
			
			class = GetValue(Line, &pos);
			_SysDebug("NOTICE class='%s'", class);
			
			if( Line[pos] == ':' ) {
				message = Line + pos + 1;
			}
			else {
				message = GetValue(Line, &pos);
			}
			
			//printf("[%s] NOTICE %s: %s\n", Server->Name, ident, message);
			char *ident_bang = strchr(ident, '!');
			if( ident_bang ) {
				*ident_bang = '\0';
			}
			Message_Append(Server, MSG_TYPE_NOTICE, ident, "", message);
		}
		else if( strcmp(cmd, "PRIVMSG") == 0 )
		{
			char	*dest, *message;
			dest = GetValue(Line, &pos);
			
			if( Line[pos] == ':' ) {
				message = Line + pos + 1;
			}
			else {
				message = GetValue(Line, &pos);
			}

			// TODO: Catch when the privmsg is addressed to the user

//			Cmd_PRIVMSG(Server, dest, ident, message);
			char *ident_bang = strchr(ident, '!');
			if( ident_bang ) {
				*ident_bang = '\0';
			}
			Message_Append(Server, MSG_TYPE_STANDARD, ident, dest, message);
		}
		else if( strcmp(cmd, "JOIN" ) == 0 )
		{
			char	*channel;
			channel = Line + pos + 1;
			Window_Create(Server, channel);
		}
		else
		{
			Message_AppendF(Server, MSG_TYPE_SERVER, "", "", "Unknown message %s (%s)\n", cmd, Line+pos);
		}
	}
	else {		
		cmd = GetValue(Line, &pos);
		
		if( strcmp(cmd, "PING") == 0 ) {
			writef(Server->FD, "PONG %s\n", gsHostname);
			
		}
		else {
			// Command to client
			Message_AppendF(NULL, MSG_TYPE_UNK, "", "", "Client Command: %s", Line);
		}
	}
}

/**
 * \brief Process incoming lines from the server
 */
int ProcessIncoming(tServer *Server)
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
		len = _SysRead(Server->FD, &Server->InBuf[Server->ReadPos], BUFSIZ - Server->ReadPos);
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

void SetCursorPos(int Row, int Col)
{
	printf("\x1B[%i;%iH", Row, Col);
}

static inline int isdigit(int ch)
{
	return '0' <= ch && ch < '9';
}

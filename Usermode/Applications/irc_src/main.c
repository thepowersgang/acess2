/*
 * Acess2 IRC Client
 */
#include <acess/sys.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <net.h>

#define BUFSIZ	1023

// === TYPES ===
typedef struct sServer {
	 int	FD;
	char	InBuf[BUFSIZ+1];
	 int	ReadPos;
} tServer;

// === PROTOTYPES ===
 int	ParseArguments(int argc, const char *argv[]);
void	ProcessIncoming(tServer *Server);
 int	writef(int FD, const char *Format, ...);
 int	OpenTCP(const char *AddressString, short PortNumber);

// === GLOBALS ===
char	*gsUsername = "root";
char	*gsHostname = "acess";
char	*gsRemoteAddress = NULL;
char	*gsRealName = "Acess2 IRC Client";
char	*gsNickname = "acess";
short	giRemotePort = 6667;

// ==== CODE ====
int main(int argc, const char *argv[], const char *envp[])
{
	 int	tmp;
	tServer	srv;
	
	memset(&srv, 0, sizeof(srv));
	
	// Parse Command line
	// - Sets the server configuration globals
	if( (tmp = ParseArguments(argc, argv)) ) {
		return tmp;
	}
	
	// Connect to the remove server
	srv.FD = OpenTCP( gsRemoteAddress, giRemotePort );
	if( srv.FD == -1 ) {
		fprintf(stderr, "Unable to create socket\n");
		return -1;
	}
	
	printf("Connection opened\n");
	ProcessIncoming(&srv);
	
	writef(srv.FD, "USER %s %s %s : %s\n", gsUsername, gsHostname, gsRemoteAddress, gsRealName);
	writef(srv.FD, "NICK %s\n", gsNickname);
	
	printf("Processing\n");
	
	for( ;; )
	{
		ProcessIncoming(&srv);
	}
	
	close(srv.FD);
	return 0;
}

/**
 * \todo Actually implement correctly :)
 */
int ParseArguments(int argc, const char *argv[])
{
	gsRemoteAddress = "130.95.13.18";	// irc.ucc.asn.au
	
	return 0;
}

void Cmd_PRIVMSG(tServer *Server, const char *Dest, const char *Src, const char *Message)
{
	printf("%p: %s => %s\t%s\n", Server, Src, Dest, Message);
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
	*Ofs = end - Src + 1;
	
	return ret;
}

/**
 */
void ParseServerLine(tServer *Server, char *Line)
{
	 int	pos = 0;
	char	*ident, *cmd;
	if( *Line == ':' ) {
		// Message
		ident = GetValue(Line, &pos);
		pos ++;	// Space
		cmd = GetValue(Line, &pos);
		
		if( strcmp(cmd, "PRIVMSG") == 0 ) {
			char	*dest, *message;
			pos ++;
			dest = GetValue(Line, &pos);
			pos ++;
			if( Line[pos] == ':' ) {
				message = Line + pos + 1;
			}
			else {
				message = GetValue(Line, &pos);
			}
			Cmd_PRIVMSG(Server, dest, ident, message);
		}
	}
	else {
		// Command to client
	}
}

/**
 * \brief Process incoming lines from the server
 */
void ProcessIncoming(tServer *Server)
{	
	char	*ptr, *newline;
	 int	len;
	
	// While there is data in the buffer, read it into user memory and 
	// process it line by line
	// ioctl#8 on a TCP client gets the number of bytes in the recieve buffer
	// - Used to avoid blocking
	#if NON_BLOCK_READ
	while( (len = ioctl(Server->FD, 8, NULL)) > 0 )
	{
	#endif
		// Read data
		len = read(Server->FD, BUFSIZ - Server->ReadPos, &Server->InBuf[Server->ReadPos]);
		Server->InBuf[Server->ReadPos + len] = '\0';
		
		// Break into lines
		ptr = Server->InBuf;
		while( (newline = strchr(ptr, '\n')) )
		{
			*newline = '\0';
			printf("%s\n", ptr);
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
	len = vsnprintf(NULL, 1000, Format, args);
	va_end(args);
	
	{
		char	buf[len+1];
		va_start(args, Format);
		vsnprintf(buf, len+1, Format, args);
		va_end(args);
		
		return write(FD, len, buf);
	}
}

/**
 * \brief Initialise a TCP connection to \a AddressString on port \a PortNumber
 */
int OpenTCP(const char *AddressString, short PortNumber)
{
	 int	fd, addrType;
	char	*iface;
	char	addrBuffer[8];
	
	// Parse IP Address
	addrType = Net_ParseAddress(AddressString, addrBuffer);
	if( addrType == 0 ) {
		fprintf(stderr, "Unable to parse '%s' as an IP address\n", AddressString);
		return -1;
	}
	
	// Finds the interface for the destination address
	iface = Net_GetInterface(addrType, addrBuffer);
	if( iface == NULL ) {
		fprintf(stderr, "Unable to find a route to '%s'\n", AddressString);
		return -1;
	}
	
	printf("iface = '%s'\n", iface);
	
	// Open client socket
	// TODO: Move this out to libnet?
	{
		 int	len = snprintf(NULL, 100, "/Devices/ip/%s/tcpc", iface);
		char	path[len+1];
		snprintf(path, 100, "/Devices/ip/%s/tcpc", iface);
		fd = open(path, OPENFLAG_READ|OPENFLAG_WRITE);
	}
	
	free(iface);
	
	if( fd == -1 ) {
		fprintf(stderr, "Unable to open TCP Client for reading\n");
		return -1;
	}
	
	// Set remote port and address
	printf("Setting port and remote address\n");
	ioctl(fd, 5, &PortNumber);
	ioctl(fd, 6, addrBuffer);
	
	// Connect
	printf("Initiating connection\n");
	if( ioctl(fd, 7, NULL) == 0 ) {
		// Shouldn't happen :(
		fprintf(stderr, "Unable to start connection\n");
		return -1;
	}
	
	// Return descriptor
	return fd;
}

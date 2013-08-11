/*
 * Acess2 Command-line HTTP Client (wget)
 * - By John Hodge
 *
 * main.c
 */
#include <stdlib.h>
#include <stdio.h>
#include <net.h>
#include <uri.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>	// struct sockaddr_in
#include <acess/sys.h>	// _SysDebug

enum eProcols
{
	PROTO_NONE,
	PROTO_HTTP,
	PROTO_HTTPS
};

const char	**gasURLs;
 int	giNumURLs;

 int	main(int argc, char *argv[]);
void	streamToFile(int Socket, const char *OutputFile, int bAppend, size_t bytes_wanted, char *inbuf, size_t ofs, size_t len);
 int	_ParseHeaderLine(char *Line, int State, size_t *Size);
void	writef(int fd, const char *format, ...);

int main(int argc, char *argv[])
{
	 int	proto, rv;
	gasURLs = malloc( (argc - 1) * sizeof(*gasURLs) );
	
	// Parse arguments
	for(int i = 1; i < argc; i ++ )
	{
		char	*arg = argv[i];
		if( arg[0] != '-' ) {
			// URL
			gasURLs[giNumURLs++] = arg;
		}
		else if( arg[1] != '-') {
			// Short arg
		}
		else {
			// Long arg
		}
	}

	// Do the download
	for( int i = 0; i < giNumURLs; i ++ )
	{
		char	*outfile = NULL;
		tURI	*uri = URI_Parse(gasURLs[i]);
		struct addrinfo	*addrinfo;

		if( !uri ) {
			fprintf(stderr, "'%s' is not a valid URL", gasURLs[i]);
			continue ;
		}		

		printf("Proto: %s, Host: %s, Path: %s\n", uri->Proto, uri->Host, uri->Path);

		if( uri->Path[0] == '\0' || uri->Path[strlen(uri->Path)-1] == '/' )
			outfile = "index.html";
		else {
			outfile = strrchr(uri->Path, '/');
			if( !outfile )
				outfile = uri->Path;
			else
				outfile += 1;
		}

		if( strcmp(uri->Proto, "http") == 0 ) {
			proto = PROTO_HTTP;
		}
		else if( strcmp(uri->Proto, "https") == 0 ) {
			proto = PROTO_HTTPS;
		}
		else {
			// Unknown
			fprintf(stderr, "Unknown protocol '%s'\n", uri->Proto);
			free(uri);
			continue ;
		}

		if( proto != PROTO_HTTP ) {
			fprintf(stderr, "TODO: Support protocols other than HTTP\n");
			free(uri);
			continue ;
		}

		rv = getaddrinfo(uri->Host, "http", NULL, &addrinfo);
		if( rv != 0 ) {
			fprintf(stderr, "Unable to resolve %s: %s\n", uri->Host, gai_strerror(rv));
			continue ;
		}

		for( struct addrinfo *addr = addrinfo; addr != NULL; addr = addr->ai_next )
		{
			 int	bSkipLine = 0;
			// TODO: Convert to POSIX/BSD
			// NOTE: using addr->ai_addr will break for IPv6, as there is more info before the address

			void *addr_data;
			switch(addr->ai_family)
			{
			case AF_INET:	addr_data = &((struct sockaddr_in*)addr->ai_addr)->sin_addr;	break;
			case AF_INET6:	addr_data = &((struct sockaddr_in6*)addr->ai_addr)->sin6_addr;	break;
			default:	addr_data = NULL;	break;
			}
			if( !addr_data )
				continue ;

			printf("Attempting [%s]:80\n", Net_PrintAddress(addr->ai_family, addr_data));
			
			int sock = Net_OpenSocket_TCPC(addr->ai_family, addr_data, 80);
			if( sock == -1 ) {
				continue ;
			}

			_SysDebug("Connected as %i", sock);
			
			writef(sock, "GET /%s HTTP/1.1\r\n", uri->Path);
			writef(sock, "Host: %s\r\n", uri->Host);
//			writef(sock, "Accept-Encodings: */*\r\n");
			writef(sock, "User-Agent: awget/0.1 (Acess2)\r\n");
			writef(sock, "\r\n");
			
			// Parse headers
			char	inbuf[16*1024];
			size_t	offset = 0, len = 0;
			 int	state = 0;
			size_t	bytes_wanted = -1;	// invalid
			

			inbuf[0] = '\0';
			while( state == 0 || state == 1 )
			{
				if( offset == BUFSIZ ) {
					bSkipLine = 1;
					offset = 0;
				}
				inbuf[len] = '\0';
			
				char	*eol = strchr(inbuf, '\n');
				// No end of line char? read some more
				if( eol == NULL ) {
					// TODO: Handle -1 return
					len += read(sock, inbuf + offset, BUFSIZ - 1 - offset);
					continue ;
				}

				// abuse offset as the end of the string	
				offset = (eol - inbuf) + 1;

				// Clear EOL bytes
				*eol = '\0';
				// Nuke the \r
				if( eol - 1 >= inbuf )
					eol[-1] = '\0';
				
				if( !bSkipLine )
					state = _ParseHeaderLine(inbuf, state, &bytes_wanted);
		
				// Move unused data down in memory	
				len -= offset;
				memmove( inbuf, inbuf + offset, BUFSIZ - offset );
				offset = len;
			}

			if( state == 2 )
			{
				_SysDebug("RXing %i bytes to '%s'", bytes_wanted, outfile);
				streamToFile(sock, outfile, 0, bytes_wanted, inbuf, len, sizeof(inbuf));
			}
			
			_SysDebug("Closing socket");
			close(sock);
			break ;
		}

		free(uri);
	}
	
	return 0;
}

void streamToFile(int Socket, const char *OutputFile, int bAppend, size_t bytes_wanted, char *inbuf, size_t len, size_t buflen)
{
	 int	outfd = open(OutputFile, O_WRONLY|O_CREAT, 0666);
	if( outfd == -1 ) {
		fprintf(stderr, "Unable to open '%s' for writing\n", OutputFile);
		return ;
	}
	
	int64_t	start_time = _SysTimestamp();
	size_t	bytes_seen = 0;
	// Write the remainder of the buffer
	do
	{
		write(outfd, inbuf, len);
		bytes_seen += len;
		_SysDebug("%i/%i bytes done", bytes_seen, bytes_wanted);
		printf("%7i/%7i KiB (%iKiB/s)\r",
			bytes_seen/1024, bytes_wanted/1024,
			bytes_seen/(_SysTimestamp() - start_time)
			);
		fflush(stdout);
		
		len = read(Socket, inbuf, buflen);
	} while( bytes_seen < bytes_wanted && len > 0 );
	close(outfd);
	printf("%i KiB done in %is (%i KiB/s)\n",
		bytes_seen/1024,
		(_SysTimestamp() - start_time)/1000,
		bytes_seen/(_SysTimestamp() - start_time)
		);
}

int _ParseHeaderLine(char *Line, int State, size_t *Size)
{
	_SysDebug("Header - %s", Line);
	// First line (Status and version)
	if( State == 0 )
	{
		// HACK - assumes HTTP/1.1 (well, 9 chars before status)
		switch( atoi(Line + 9) )
		{
		case 200:
			// All good!
			return 1;
		default:
			fprintf(stderr, "Unknown HTTP status - %s\n", Line + 9);
			return -1;
		}
	}
	// Last line?
	else if( Line[0] == '\0' )
	{
		return 2;
	}
	// Body lines
	else
	{
		char	*value;
		char	*colon = strchr(Line, ':');
		if(colon == NULL)	return 1;

		*colon = '\0';
		value = colon + 2;
		if( strcmp(Line, "Content-Length") == 0 ) {
			*Size = atoi(value);
		}
		else {
			printf("Ignorning header '%s' = '%s'\n", Line, value);
		}

		return 1;
	}
}

void writef(int fd, const char *format, ...)
{
	va_list	args;
	size_t	len;
	
	va_start(args, format);
	len = vsnprintf(NULL, 0, format, args);
	va_end(args);
	
	char data[len + 1];
	va_start(args, format);
	vsnprintf(data, len+1, format, args);
	va_end(args);
	
	write(fd, data, len);
}


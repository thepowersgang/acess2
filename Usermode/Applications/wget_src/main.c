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

enum eProcols
{
	PROTO_NONE,
	PROTO_HTTP,
	PROTO_HTTPS
};

const char	**gasURLs;
 int	giNumURLs;

int _ParseHeaderLine(char *Line, int State, size_t *Size);

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
		char	*outfile;
		tURI	*uri = URI_Parse(gasURLs[i]);
		struct addrinfo	*addrinfo;

		if( !uri ) {
			fprintf(stderr, "'%s' is not a valid URL", gasURLs[i]);
			continue ;
		}		

		printf("Proto: %s, Host: %s, Path: %s\n", uri->Proto, uri->Host, uri->Path);

		if( uri->Path[0] == '\0' )
			outfile = "index.html";
		else
			outfile = strrchr(uri->Path, '/') + 1;
		if( !outfile )
			outfile = uri->Path;

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

		rv = getaddrinfo(uri->Host, NULL, NULL, &addrinfo);
		if( rv != 0 ) {
			fprintf(stderr, "Unable to resolve %s: %s\n", uri->Host, gai_strerror(rv));
			continue ;
		}

		for( struct addrinfo *addr = addrinfo; addr != NULL; addr = addr->ai_next )
		{
			 int	bSkipLine = 0;
			// TODO: Convert to POSIX/BSD
			 int	sock = Net_OpenSocket(addr->ai_family, addr->ai_addr, "tcpc");
			if( sock == -1 )	continue ;

			writef(sock, "GET /%s HTTP/1.1\r\n", uri->Path);
			writef(sock, "Host: %s\r\n", uri->Host);
//			writef(sock, "Accept-Encodings: */*\r\n");
			writef(sock, "User-Agent: awget/0.1 (Acess2)\r\n");
			writef(sock, "\r\n");
			
			// Parse headers
			char	inbuf[BUFSIZ+1];
			size_t	offset = 0;
			 int	state = 0;
			size_t	bytes_seen = 0;
			size_t	bytes_wanted = -1;	// invalid
			
			while( state == 0 || state == 1 )
			{
				if( offset == sizeof(inbuf) ) {
					bSkipLine = 1;
					offset = 0;
				}
				// TODO: Handle -1 return
				offset += read(sock, inbuf + offset, sizeof(inbuf) - offset);
				inbuf[offset] = 0;
				
				char	*eol = strchr(inbuf, '\n');
				// No end of line char? read some more
				if( eol == NULL )	continue ;
				// Update write offset	
				offset = eol + 1 - inbuf;
				
				// Clear EOL bytes
				*eol = '\0';
				// Nuke the \r
				if( eol - 1 >= inbuf )
					eol[-1] = '\0';
					
				
				if( !bSkipLine )
					state = _ParseHeaderLine(inbuf, state, &bytes_wanted);
				
				memmove( inbuf, inbuf + offset, sizeof(inbuf) - offset );
				offset = 0;
			}

			if( state == 2 )
			{
				 int	outfd = open(outfile, O_WR|O_CREAT, 0666);
				if( outfd != -1 )
				{
					while( bytes_seen < bytes_wanted )
					{
						// Abuses offset as read byte count
						offset = read(sock, inbuf, sizeof(inbuf));
						write(outfd, inbuf, offset);
						bytes_seen += offset;
					}
					close(outfd);
				}
			}
			
			close(sock);
			break ;
		}

		free(uri);
	}
}

int _ParseHeaderLine(char *Line, int State, size_t *Size)
{
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
		char	*colon = strchr(Line, ':');
		if(colon == NULL)	return 1;

		*colon = '\0';
		if( strcmp(Line, "Content-Length") == 0 ) {
			*Size = atoi(colon + 1);
		}
		else {
			printf("Ignorning header '%s' = '%s'\n", Line, colon+1);
		}

		return 1;
	}
}


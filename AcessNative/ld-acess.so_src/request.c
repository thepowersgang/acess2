/*
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#ifdef __WIN32__
# include <windows.h>
# include <winsock.h>
#else
# include <unistd.h>
# include <sys/socket.h>
# include <netinet/in.h>
#endif

#define	SERVER_PORT	0xACE

// === GLOBALS ===
#ifdef __WIN32__
WSADATA	gWinsock;
SOCKET	gSocket;
#else
 int	gSocket;
# define INVALID_SOCKET -1
#endif

// === CODE ===
int _InitSyscalls()
{
	struct sockaddr_in	server;
	struct sockaddr_in	client;
	
	#ifdef __WIN32__
	/* Open windows connection */
	if (WSAStartup(0x0101, &gWinsock) != 0)
	{
		fprintf(stderr, "Could not open Windows connection.\n");
		exit(0);
	}
	#endif
	
	// Open UDP Connection
	gSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (gSocket == INVALID_SOCKET)
	{
		fprintf(stderr, "Could not create socket.\n");
		#if __WIN32__
		WSACleanup();
		#endif
		exit(0);
	}
	
	// Set server address
	memset((void *)&server, '\0', sizeof(struct sockaddr_in));
	server.sin_family = AF_INET;
	server.sin_port = htons(SERVER_PORT);
	server.sin_addr.s_addr = htonl(0x7F00001);
	
	// Set client address
	memset((void *)&client, '\0', sizeof(struct sockaddr_in));
	client.sin_family = AF_INET;
	client.sin_port = htons(0);
	client.sin_addr.s_addr = htonl(0x7F00001);
	
	// Bind
	if( bind(gSocket, (struct sockaddr *)&client, sizeof(struct sockaddr_in)) == -1 )
	{
		fprintf(stderr, "Cannot bind address to socket.\n");
		#if __WIN32__
		closesocket(gSocket);
		WSACleanup();
		#else
		close(gSocket);
		#endif
		exit(0);
	}
	return 0;
}
#if 0
int _Syscall(const char *ArgTypes, ...)
{
	return 0;
}
#endif

/*
 */
#include <windows.h>
#include <stdio.h>
#include <winsock.h>

#define	SERVER_PORT	0xACE

// === GLOBALS ===
#ifdef __WIN32__
WSADATA	gWinsock;
SOCKET	gSocket;
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
		WSACleanup();
		exit(0);
	}
	
	// Set server address
	memset((void *)&server, '\0', sizeof(struct sockaddr_in));
	server.sin_family = AF_INET;
	server.sin_port = htons(SERVER_PORT);
	server.sin_addr.S_un.S_un_b.s_b1 = (unsigned char)127;
	server.sin_addr.S_un.S_un_b.s_b2 = (unsigned char)0;
	server.sin_addr.S_un.S_un_b.s_b3 = (unsigned char)0;
	server.sin_addr.S_un.S_un_b.s_b4 = (unsigned char)1;
	
	// Set client address
	memset((void *)&client, '\0', sizeof(struct sockaddr_in));
	client.sin_family = AF_INET;
	client.sin_port = htons(0);
	client.sin_addr.S_un.S_un_b.s_b1 = (unsigned char)127;
	client.sin_addr.S_un.S_un_b.s_b2 = (unsigned char)0;
	client.sin_addr.S_un.S_un_b.s_b3 = (unsigned char)0;
	client.sin_addr.S_un.S_un_b.s_b4 = (unsigned char)1;
	
	// Bind
	if( bind(gSocket, (struct sockaddr *)&client, sizeof(struct sockaddr_in)) == -1 )
	{
		fprintf(stderr, "Cannot bind address to socket.\n");
		closesocket(gSocket);
		WSACleanup();
		exit(0);
	}
}

int _Syscall(const char *ArgTypes, ...)
{
	return 0;
}

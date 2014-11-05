/*
 * AcessNative ld-acess dynamic linker
 * - By John Hodge (thePowersGang)
 *
 * request.c
 * - IPC interface
 */
#define DEBUG	0

#if DEBUG
# define DEBUG_S	printf
#else
# define DEBUG_S(...)
# define DONT_INCLUDE_SYSCALL_NAMES
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>
#ifdef __WIN32__
# include <windows.h>
# include <winsock.h>
#else
# include <unistd.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <sys/select.h>
#endif
#include "request.h"
#include "../syscalls.h"

#define USE_TCP	1

// === PROTOTYPES ===
void	SendData(void *Data, int Length);
 int	ReadData(void *Dest, int MaxLen, int Timeout);

// === GLOBALS ===
#ifdef __WIN32__
WSADATA	gWinsock;
SOCKET	gSocket = INVALID_SOCKET;
#else
# define INVALID_SOCKET -1
 int	gSocket = INVALID_SOCKET;
#endif
// Client ID to pass to server
// TODO: Implement such that each thread gets a different one
 int	giSyscall_ClientID = 0;
struct sockaddr_in	gSyscall_ServerAddr;

// === CODE ===
void Request_Preinit(void)
{
	// Set server address
	memset((void *)&gSyscall_ServerAddr, '\0', sizeof(struct sockaddr_in));
	gSyscall_ServerAddr.sin_family = AF_INET;
	gSyscall_ServerAddr.sin_port = htons(SERVER_PORT);
	gSyscall_ServerAddr.sin_addr.s_addr = htonl(0x7F000001);
}

int _InitSyscalls(void)
{
	#ifdef __WIN32__
	/* Open windows connection */
	if (WSAStartup(0x0101, &gWinsock) != 0)
	{
		fprintf(stderr, "Could not open Windows connection.\n");
		exit(0);
	}
	#endif
	
	#if USE_TCP
	// Open TCP Connection
	gSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	#else
	// Open UDP Connection
	gSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	#endif
	if (gSocket == INVALID_SOCKET)
	{
		fprintf(stderr, "Could not create socket.\n");
		#if __WIN32__
		WSACleanup();
		#endif
		exit(0);
	}
	
	#if USE_TCP
	if( connect(gSocket, (struct sockaddr *)&gSyscall_ServerAddr, sizeof(struct sockaddr_in)) < 0 )
	{
		fprintf(stderr, "[ERROR -] Cannot connect to server (localhost:%i)\n", SERVER_PORT);
		perror("_InitSyscalls");
		#if __WIN32__
		fprintf(stderr, "[ERROR -] - WSAGetLastError said %i", WSAGetLastError());
		closesocket(gSocket);
		WSACleanup();
		#else
		close(gSocket);
		#endif
		exit(0);
	}
	#endif
	
	#if 0
	// Set client address
	memset((void *)&client, '\0', sizeof(struct sockaddr_in));
	client.sin_family = AF_INET;
	client.sin_port = htons(0);
	client.sin_addr.s_addr = htonl(0x7F000001);
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
	#endif
	
	#if USE_TCP
	{
		tRequestAuthHdr auth;
		auth.pid = giSyscall_ClientID;
		auth.key = 0;
		SendData(&auth, sizeof(auth));
		int len = ReadData(&auth, sizeof(auth), 5);
		if( len == 0 ) { 
			fprintf(stderr, "Timeout waiting for auth response\n");
			exit(-1);
		}
		giSyscall_ClientID = auth.pid;
	}
	#else
	// Ask server for a client ID
	if( !giSyscall_ClientID )
	{
		tRequestHeader	req;
		 int	len;
		req.ClientID = 0;
		req.CallID = 0;
		req.NParams = 0;
		
		SendData(&req, sizeof(req));
		
		len = ReadData(&req, sizeof(req), 5);
		if( len == 0 ) {
			fprintf(stderr, "Unable to connect to server (localhost:%i)\n", SERVER_PORT);
			exit(-1);
		}
		
		giSyscall_ClientID = req.ClientID;
	}
	#endif
	
	return 0;
}

/**
 * \brief Close the syscall socket
 * \note Used in acess_fork to get a different port number
 */
void _CloseSyscalls(void)
{
	#if __WIN32__
	closesocket(gSocket);
	WSACleanup();
	#else
	close(gSocket);
	#endif
}

int SendRequest(tRequestHeader *Request, int RequestSize, int ResponseSize)
{
	if( gSocket == INVALID_SOCKET )
	{
		_InitSyscalls();		
	}
	
	// Set header
	Request->ClientID = giSyscall_ClientID;
	
	#if 0
	{
		for(i=0;i<RequestSize;i++)
		{
			printf("%02x ", ((uint8_t*)Request)[i]);
			if( i % 16 == 15 )	printf("\n");
		}
		printf("\n");
	}
	#endif
	#if DEBUG
	{
		 int	i;
		char	*data = (char*)&Request->Params[Request->NParams];
		DEBUG_S("Request #%i (%s) -", Request->CallID, casSYSCALL_NAMES[Request->CallID]);
		for( i = 0; i < Request->NParams; i ++ )
		{
			switch(Request->Params[i].Type)
			{
			case ARG_TYPE_INT32:
				DEBUG_S(" 0x%08x", *(uint32_t*)data);
				data += sizeof(uint32_t);
				break;
			case ARG_TYPE_INT64:
				DEBUG_S(" 0x%016"PRIx64"", *(uint64_t*)data);
				data += sizeof(uint64_t);
				break;
			case ARG_TYPE_STRING:
				DEBUG_S(" '%s'", (char*)data);
				data += Request->Params[i].Length;
				break;
			case ARG_TYPE_DATA:
				DEBUG_S(" %p:0x%x", (char*)data, Request->Params[i].Length);
				if( !(Request->Params[i].Flags & ARG_FLAG_ZEROED) )
					data += Request->Params[i].Length;
				break;
			}
		}
		DEBUG_S("\n");
	}
	#endif
	
	// Send it off
	SendData(Request, RequestSize);

	// Wait for a response (no timeout)
	ReadData(Request, sizeof(*Request), 0);
	
	size_t	recvbytes = sizeof(*Request);
	// TODO: Sanity
	size_t	expbytes = Request->MessageLength;
	char	*ptr = (void*)Request->Params;
	while( recvbytes < expbytes )
	{
		size_t	len = ReadData(ptr, expbytes - recvbytes, 1000);
		if( len == -1 ) {
			return -1;
		}
		recvbytes += len;
		ptr += len;
	}
	if( recvbytes > expbytes ) {
		// TODO: Warning
	}
	
	#if DEBUG
	{
		 int	i;
		char	*data = (char*)&Request->Params[Request->NParams];
		DEBUG_S(" Reply:");
		for( i = 0; i < Request->NParams; i ++ )
		{
			switch(Request->Params[i].Type)
			{
			case ARG_TYPE_INT32:
				DEBUG_S(" 0x%08x", *(uint32_t*)data);
				data += sizeof(uint32_t);
				break;
			case ARG_TYPE_INT64:
				DEBUG_S(" 0x%016"PRIx64"", *(uint64_t*)data);
				data += sizeof(uint64_t);
				break;
			case ARG_TYPE_STRING:
				DEBUG_S(" '%s'", (char*)data);
				data += Request->Params[i].Length;
				break;
			case ARG_TYPE_DATA:
				DEBUG_S(" %p:0x%x", (char*)data, Request->Params[i].Length);
				if( !(Request->Params[i].Flags & ARG_FLAG_ZEROED) )
					data += Request->Params[i].Length;
				break;
			}
		}
		DEBUG_S("\n");
	}
	#endif
	return recvbytes;
}

void SendData(void *Data, int Length)
{
	 int	len;
	
	#if USE_TCP
	len = send(gSocket, Data, Length, 0);
	#else
	len = sendto(gSocket, Data, Length, 0,
		(struct sockaddr*)&gSyscall_ServerAddr, sizeof(gSyscall_ServerAddr));
	#endif
	
	if( len != Length ) {
		fprintf(stderr, "[ERROR %i] ", giSyscall_ClientID);
		perror("SendData");
		exit(-1);
	}
}

int ReadData(void *Dest, int MaxLength, int Timeout)
{
	 int	ret;
	fd_set	fds;
	struct timeval	tv;
	struct timeval	*timeoutPtr;
	
	FD_ZERO(&fds);
	FD_SET(gSocket, &fds);
	
	if( Timeout ) {
		tv.tv_sec = Timeout;
		tv.tv_usec = 0;
		timeoutPtr = &tv;
	}
	else {
		timeoutPtr = NULL;
	}
	
	ret = select(gSocket+1, &fds, NULL, NULL, timeoutPtr);
	if( ret == -1 ) {
		fprintf(stderr, "[ERROR %i] ", giSyscall_ClientID);
		perror("ReadData - select");
		exit(-1);
	}
	
	if( !ret ) {
		printf("[ERROR %i] Timeout reading from socket\n", giSyscall_ClientID);
		return -2;	// Timeout
	}
	
	#if USE_TCP
	ret = recv(gSocket, Dest, MaxLength, 0);
	#else
	ret = recvfrom(gSocket, Dest, MaxLength, 0, NULL, 0);
	#endif
	
	if( ret < 0 ) {
		fprintf(stderr, "[ERROR %i] ", giSyscall_ClientID);
		perror("ReadData");
		exit(-1);
	}
	if( ret == 0 ) {
		fprintf(stderr, "[ERROR %i] Connection closed.\n", giSyscall_ClientID);
		#if __WIN32__
		closesocket(gSocket);
		#else
		close(gSocket);
		#endif
		exit(0);
	}
	
	DEBUG_S("%i bytes read from socket\n", ret);
	
	return ret;
}

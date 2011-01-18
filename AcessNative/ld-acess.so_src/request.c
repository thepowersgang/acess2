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
#include "request.h"
#include "../syscalls.h"

#define	SERVER_PORT	0xACE

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
static int	siSyscall_ClientID = 0;

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

int SendRequest(int RequestID, int NumOutput, tOutValue **Output, int NumInput, tInValue **Input)
{
	tRequestHeader	*request;
	tRequestValue	*value;
	char	*data;
	 int	requestLen;
	 int	i;
	
	// See ../syscalls.h for details of request format
	requestLen = sizeof(tRequestHeader) + (NumOutput + NumInput) * sizeof(tRequestValue);
	
	// Get total param length
	for( i = 0; i < NumOutput; i ++ )
		requestLen += Output[i]->Length;
	
	// Allocate request
	request = malloc( requestLen );
	value = request->Params;
	data = (char*)&request->Params[ NumOutput + NumInput ];
	
	// Set header
	request->ClientID = siSyscall_ClientID;
	request->CallID = RequestID;	// Syscall
	request->NParams = NumOutput;
	request->NReturn = NumInput;
	
	// Set parameters
	for( i = 0; i < NumOutput; i ++ )
	{
		switch(Output[i]->Type)
		{
		case 'i':	value->Type = ARG_TYPE_INT32;	break;
		case 'I':	value->Type = ARG_TYPE_INT64;	break;
		case 'd':	value->Type = ARG_TYPE_DATA;	break;
		default:
			return -1;
		}
		value->Length = Output[i]->Length;
		
		memcpy(data, Output[i]->Data, Output[i]->Length);
		
		data += Output[i]->Length;
	}
	
	// Set return values
	for( i = 0; i < NumInput; i ++ )
	{
		switch(Input[i]->Type)
		{
		case 'i':	value->Type = ARG_TYPE_INT32;	break;
		case 'I':	value->Type = ARG_TYPE_INT64;	break;
		case 'd':	value->Type = ARG_TYPE_DATA;	break;
		default:
			return -1;
		}
		value->Length = Input[i]->Length;
	}
	
	// Send it off
	send(gSocket, request, requestLen, 0);
	
	// Wait for a response
	recv(gSocket, request, requestLen, 0);
	
	// Parse response out
	
	return 0;
}

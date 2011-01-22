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
	
	// Open TCP Connection
	gSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	// Open UDP Connection
	//gSocket = socket(AF_INET, SOCK_DGRAM, 0);
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
	
	if( connect(gSocket, (struct sockaddr *)&server, sizeof(struct sockaddr_in)) < 0 )
	{
		fprintf(stderr, "Cannot connect to server (localhost:%i)\n", SERVER_PORT);
		perror("_InitSyscalls");
		#if __WIN32__
		closesocket(gSocket);
		WSACleanup();
		#else
		close(gSocket);
		#endif
		exit(0);
	}
	
	#if 0
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
	
	return 0;
}

int SendRequest(int RequestID, int NumOutput, tOutValue **Output, int NumInput, tInValue **Input)
{
	tRequestHeader	*request;
	tRequestValue	*value;
	char	*data;
	 int	requestLen;
	 int	i;
	
	if( gSocket == INVALID_SOCKET )
	{
		_InitSyscalls();		
	}
	
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
		case 's':	value->Type = ARG_TYPE_DATA;	break;
		default:
			fprintf(stderr, __FILE__" SendRequest: Unknown output type '%c'\n",
				Output[i]->Type);
			return -1;
		}
		value->Length = Output[i]->Length;
		
		memcpy(data, Output[i]->Data, Output[i]->Length);
		
		value ++;
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
			fprintf(stderr, " SendRequest: Unknown input type '%c'\n",
				Input[i]->Type);
			return -1;
		}
		value->Length = Input[i]->Length;
		value ++;
	}
	#if 0
	printf("value = %p\n", value);
	{
		for(i=0;i<requestLen;i++)
		{
			printf("%02x ", ((uint8_t*)request)[i]);
			if( i % 16 == 15 )	printf("\n");
		}
		printf("\n");
	}
	#endif
	
	// Send it off
	if( send(gSocket, request, requestLen, 0) != requestLen ) {
		fprintf(stderr, "SendRequest: send() failed\n");
		perror("SendRequest - send");
		free( request );
		return -1;
	}
	
	// Wait for a response
	requestLen = recv(gSocket, request, requestLen, 0);
	if( requestLen < 0 ) {
		fprintf(stderr, "SendRequest: revc() failed\n");
		perror("SendRequest - recv");
		free( request );
		return -1;
	}
	
	// Parse response out
	if( request->NParams != NumInput ) {
		fprintf(stderr, "SendRequest: Unexpected number of values retured (%i, exp %i)\n",
			request->NParams, NumInput
			);
		free( request );
		return -1;
	}
	
	// Free memory
	free( request );
	
	return 0;
}

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

#define USE_TCP	0

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
int _InitSyscalls()
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
	
	// Set server address
	memset((void *)&gSyscall_ServerAddr, '\0', sizeof(struct sockaddr_in));
	gSyscall_ServerAddr.sin_family = AF_INET;
	gSyscall_ServerAddr.sin_port = htons(SERVER_PORT);
	gSyscall_ServerAddr.sin_addr.s_addr = htonl(0x7F000001);
	
	#if 0
	// Set client address
	memset((void *)&client, '\0', sizeof(struct sockaddr_in));
	client.sin_family = AF_INET;
	client.sin_port = htons(0);
	client.sin_addr.s_addr = htonl(0x7F000001);
	#endif
	
	#if USE_TCP
	if( connect(gSocket, (struct sockaddr *)&gSyscall_ServerAddr, sizeof(struct sockaddr_in)) < 0 )
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
	#endif
	
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
	
	#if !USE_TCP
	// Ask server for a client ID
	{
		tRequestHeader	req;
		 int	len;
		req.ClientID = 0;
		req.CallID = 0;
		req.NParams = 0;
		req.NReturn = 0;
		
		SendData(&req, sizeof(req));
		
		len = ReadData(&req, sizeof(req), 5);
		
		giSyscall_ClientID = req.ClientID;
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
	request->ClientID = giSyscall_ClientID;
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
	SendData(request, requestLen);
	
	// Wait for a response (no timeout)
	requestLen = ReadData(request, requestLen, -1);
	
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

void SendData(void *Data, int Length)
{
	 int	len;
	
	#if USE_TCP
	len = send(Data, Length, 0);
	#else
	len = sendto(gSocket, Data, Length, 0,
		(struct sockaddr*)&gSyscall_ServerAddr, sizeof(gSyscall_ServerAddr));
	#endif
	
	if( len != Length ) {
		perror("SendData");
		exit(-1);
	}
}

int ReadData(void *Dest, int MaxLength, int Timeout)
{
	 int	ret;
	fd_set	fds;
	struct timeval	tv;
	
	FD_ZERO(&fds);
	FD_SET(gSocket, &fds);
	
	tv.tv_sec = Timeout;
	tv.tv_usec = 0;
	
	ret = select(1, &fds, NULL, NULL, &tv);
	if( ret == -1 ) {
		perror("ReadData - select");
		exit(-1);
	}
	
	if( !ret ) {
		printf("Timeout reading from socket\n");
		return 0;	// Timeout
	}
	
	#if USE_TCP
	ret = recv(gSocket, Dest, MaxLength, 0);
	#else
	ret = recvfrom(gSocket, Dest, MaxLength, 0, NULL, 0);
	#endif
	
	if( ret < 0 ) {
		perror("ReadData");
		exit(-1);
	}
	
	return ret;
}

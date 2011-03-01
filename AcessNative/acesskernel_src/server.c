/*
 * Acess2 Native Kernel
 * - Acess kernel emulation on another OS using SDL and UDP
 *
 * Syscall Server
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL/SDL.h>
#ifdef __WIN32__
# include <windows.h>
# include <winsock.h>
#else
# include <unistd.h>
# include <sys/socket.h>
# include <netinet/in.h>
#endif
#include "../syscalls.h"

#define	USE_TCP	0
#define MAX_CLIENTS	16

// === TYPES ===
typedef struct {
	 int	ClientID;
	SDL_Thread	*WorkerThread;
	#if USE_TCP
	#else
	tRequestHeader	*CurrentRequest;
	struct sockaddr_in	ClientAddr;
	SDL_cond	*WaitFlag;
	SDL_mutex	*Mutex;
	#endif
}	tClient;

// === IMPORTS ===
extern tRequestHeader *SyscallRecieve(tRequestHeader *Request, int *ReturnLength);

// === PROTOTYPES ===
tClient	*Server_GetClient(int ClientID);
 int	Server_WorkerThread(void *ClientPtr);
 int	SyscallServer(void);

// === GLOBALS ===
#ifdef __WIN32__
WSADATA	gWinsock;
SOCKET	gSocket = INVALID_SOCKET;
#else
# define INVALID_SOCKET -1
 int	gSocket = INVALID_SOCKET;
#endif
 int	giServer_NextClientID = 1;
tClient	gaServer_Clients[MAX_CLIENTS];

// === CODE ===
int Server_GetClientID(void)
{
	 int	i;
	Uint32	thisId = SDL_ThreadID();
	
	for( i = 0; i < MAX_CLIENTS; i ++ )
	{
		if( SDL_GetThreadID(gaServer_Clients[i].WorkerThread) == thisId )
			return gaServer_Clients[i].ClientID;
	}
	
	fprintf(stderr, "ERROR: Server_GetClientID - Thread is not allocated\n");
	
	return 0;
}

tClient *Server_GetClient(int ClientID)
{
	tClient	*ret = NULL;
	 int	i;
	
	for( i = 0; i < MAX_CLIENTS; i ++ )
	{
		if( gaServer_Clients[i].ClientID == ClientID ) {
			ret = &gaServer_Clients[i];
			break;
		}
	}
	
	// Uh oh, no free slots
	// TODO: Dynamic allocation
	if( !ret )
		return NULL;
	
	if( ClientID == 0 )
	{
		ret->ClientID = giServer_NextClientID ++;
		ret->CurrentRequest = NULL;
		
		if( !ret->WorkerThread ) {
			ret->WaitFlag = SDL_CreateCond();
			ret->Mutex = SDL_CreateMutex();
			SDL_mutexP( ret->Mutex );
			ret->WorkerThread = SDL_CreateThread( Server_WorkerThread, ret );
		}
	}
	
	return &gaServer_Clients[i];
}

int Server_WorkerThread(void *ClientPtr)
{
	tClient	*Client = ClientPtr;
	tRequestHeader	*retHeader;
	tRequestHeader	errorHeader;
	 int	retSize = 0;
	 int	sentSize;
	
	#if USE_TCP
	#else
	for( ;; )
	{
		// Wait for something to do
		while( Client->CurrentRequest == NULL )
			SDL_CondWait(Client->WaitFlag, Client->Mutex);
		
		printf("Worker for %i, Job: %p\n", Client->ClientID, Client->CurrentRequest);
		
		// Get the response
		retHeader = SyscallRecieve(Client->CurrentRequest, &retSize);
		
		if( !retHeader ) {
			// Return an error to the client
			printf("Error returned by SyscallRecieve\n");
			errorHeader.CallID = Client->CurrentRequest->CallID;
			errorHeader.NParams = 0;
			retHeader = &errorHeader;
			retSize = sizeof(errorHeader);
		}
		
		// Set ID
		retHeader->ClientID = Client->ClientID;
		
		// Mark the thread as ready for another job
		Client->CurrentRequest = 0;
		
		printf("Sending %i to %x:%i\n",
			retSize, ntohl(Client->ClientAddr.sin_addr.s_addr),
			ntohs(Client->ClientAddr.sin_port)
			);
		
		// Return the data
		sentSize = sendto(gSocket, retHeader, retSize, 0,
			(struct sockaddr*)&Client->ClientAddr, sizeof(Client->ClientAddr)
			);
		if( sentSize != retSize ) {
			perror("Server_WorkerThread - send");
		}
		
		// Free allocated header
		if( retHeader != &errorHeader )
			free( retHeader );
	}
	#endif
}

int SyscallServer(void)
{
	struct sockaddr_in	server;
	
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
	memset(&server, 0, sizeof(struct sockaddr_in));
	server.sin_family = AF_INET;
	server.sin_port = htons(SERVER_PORT);
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	
	// Bind
	if( bind(gSocket, (struct sockaddr *)&server, sizeof(struct sockaddr_in)) == -1 )
	{
		fprintf(stderr, "Cannot bind address to socket.\n");
		perror("SyscallServer - bind");
		#if __WIN32__
		closesocket(gSocket);
		WSACleanup();
		#else
		close(gSocket);
		#endif
		exit(0);
	}
	
	#if USE_TCP
	listen(gSocket, 5);
	#endif
	
	Log_Notice("Syscall", "Listening on 0.0.0.0:%i\n", SERVER_PORT);
	
	// Wait for something to do :)
	for( ;; )
	{
		#if USE_TCP
		struct sockaddr_in	client;
		uint	clientSize = sizeof(client);
		 int	clientSock = accept(gSocket, (struct sockaddr*)&client, &clientSize);
		if( clientSock < 0 ) {
			perror("SyscallServer - accept");
			break ;
		}
		
		printf("Client connection %x:%i",
			ntohl(client.sin_addr), ntohs(client.sin_port)
			);
		
		#else
		char	data[BUFSIZ];
		tRequestHeader	*req = (void*)data;
		struct sockaddr_in	addr;
		uint	clientSize = sizeof(addr);
		 int	length = recvfrom(gSocket, data, BUFSIZ, 0, (struct sockaddr*)&addr, &clientSize);
		tClient	*client;
		
		if( length == -1 ) {
			perror("SyscallServer - recv");
			break;
		}
		
		// Hand off to a worker thread
		// - TODO: Actually have worker threads
		printf("%i bytes from %x:%i\n", length,
			ntohl(addr.sin_addr.s_addr), ntohs(addr.sin_port));
		
		client = Server_GetClient(req->ClientID);
		if( req->ClientID == 0 )
		{
			memcpy(&client->ClientAddr, &addr, sizeof(addr));
		}
		else if( memcmp(&client->ClientAddr, &addr, sizeof(addr)) != 0 )
		{
			printf("ClientID %i used by %x:%i\n",
				client->ClientID, ntohl(addr.sin_addr.s_addr), ntohs(addr.sin_port));
			printf(" actually owned by %x:%i\n",
				ntohl(client->ClientAddr.sin_addr.s_addr), ntohs(client->ClientAddr.sin_port));
			continue;
		}
		
		if( client->CurrentRequest ) {
			printf("Worker thread for %x:%i is busy\n",
				ntohl(client->ClientAddr.sin_addr.s_addr), ntohs(client->ClientAddr.sin_port));
			continue;
		}
		
		printf("client = %p, ClientID = %i\n", client, client->ClientID);
		
		client->CurrentRequest = req;
		SDL_CondSignal(client->WaitFlag);
		#endif
	}
	
	return -1;
}

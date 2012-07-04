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
//#include <debug.h>

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
extern int	Threads_CreateRootProcess(void);
extern void	Threads_SetThread(int TID);
// HACK: Should have these in a header
extern void	Log_Debug(const char *Subsys, const char *Message, ...);
extern void	Log_Notice(const char *Subsys, const char *Message, ...);

// === PROTOTYPES ===
tClient	*Server_GetClient(int ClientID);
 int	Server_WorkerThread(void *ClientPtr);
 int	SyscallServer(void);
 int	Server_ListenThread(void *Unused);

// === GLOBALS ===
#ifdef __WIN32__
WSADATA	gWinsock;
SOCKET	gSocket = INVALID_SOCKET;
#else
# define INVALID_SOCKET -1
 int	gSocket = INVALID_SOCKET;
#endif
tClient	gaServer_Clients[MAX_CLIENTS];
SDL_Thread	*gpServer_ListenThread;

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
	
	// Allocate an ID if needed
	if(ClientID == 0)
		ClientID = Threads_CreateRootProcess();
	
	for( i = 0; i < MAX_CLIENTS; i ++ )
	{
		if( gaServer_Clients[i].ClientID == ClientID ) {
			return &gaServer_Clients[i];
		}
		if(!ret && gaServer_Clients[i].ClientID == 0)
			ret = &gaServer_Clients[i];
	}
	
	// Uh oh, no free slots
	// TODO: Dynamic allocation
	if( !ret )
		return NULL;
	
	// Allocate a thread for the process
	ret->ClientID = ClientID;
	ret->CurrentRequest = NULL;
		
	if( !ret->WorkerThread ) {
		ret->WaitFlag = SDL_CreateCond();
		ret->Mutex = SDL_CreateMutex();
		SDL_mutexP( ret->Mutex );
		ret->WorkerThread = SDL_CreateThread( Server_WorkerThread, ret );
	}
	
	return ret;
}

int Server_WorkerThread(void *ClientPtr)
{
	tClient	*Client = ClientPtr;
	tRequestHeader	*retHeader;
	tRequestHeader	errorHeader;
	 int	retSize = 0;
	 int	sentSize;
	 int	cur_client_id = 0;
	
	#if USE_TCP
	#else
	for( ;; )
	{
		// Wait for something to do
		while( Client->CurrentRequest == NULL )
			SDL_CondWait(Client->WaitFlag, Client->Mutex);
		
		Log_Debug("AcessSrv", "Worker got message %p", Client->CurrentRequest);
		
		if(Client->ClientID != cur_client_id) {
			Threads_SetThread( Client->ClientID );
			cur_client_id = Client->ClientID;
		}
		
		// Debug
		{
			int	callid = Client->CurrentRequest->CallID;
			Log_Debug("AcessSrv", "Client %i request %i %s",
				Client->ClientID, callid,
				callid < N_SYSCALLS ? casSYSCALL_NAMES[callid] : "UNK"
				);
		}
		
		// Get the response
		retHeader = SyscallRecieve(Client->CurrentRequest, &retSize);

		if( !retHeader ) {
			// Return an error to the client
			printf("ERROR: SyscallRecieve failed\n");
			errorHeader.CallID = Client->CurrentRequest->CallID;
			errorHeader.NParams = 0;
			retHeader = &errorHeader;
			retSize = sizeof(errorHeader);
		}
		
		// Set ID
		retHeader->ClientID = Client->ClientID;
		
		// Mark the thread as ready for another job
		free(Client->CurrentRequest);
		Client->CurrentRequest = 0;
		
//		Log_Debug("AcessSrv", "Sending %i to %x:%i (Client %i)",
//			retSize, ntohl(Client->ClientAddr.sin_addr.s_addr),
//			ntohs(Client->ClientAddr.sin_port),
//			Client->ClientID
//			);
		
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
	
	Log_Notice("AcessSrv", "Listening on 0.0.0.0:%i", SERVER_PORT);
	gpServer_ListenThread = SDL_CreateThread( Server_ListenThread, NULL );
	return 0;
}

int Server_ListenThread(void *Unused)
{	
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
		
		Log("Client connection %x:%i\n",
			ntohl(client.sin_addr), ntohs(client.sin_port)
			);
		
		#else
		char	data[BUFSIZ];
		tRequestHeader	*req = (void*)data;
		struct sockaddr_in	addr;
		uint	clientSize = sizeof(addr);
		 int	length;
		tClient	*client;
		
		length = recvfrom(gSocket, data, BUFSIZ, 0, (struct sockaddr*)&addr, &clientSize);
		
		if( length == -1 ) {
			perror("SyscallServer - recv");
			break;
		}
		
		// Hand off to a worker thread
		// - TODO: Actually have worker threads
//		Log_Debug("Server", "%i bytes from %x:%i", length,
//			ntohl(addr.sin_addr.s_addr), ntohs(addr.sin_port));
		
		client = Server_GetClient(req->ClientID);
		// NOTE: Hack - Should check if all zero
		if( req->ClientID == 0 || client->ClientAddr.sin_port == 0 )
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
		
		Log_Debug("AcessSrv", "Message from Client %i (%p)",
			client->ClientID, client);

		// Make a copy of the request data	
		req = malloc(length);
		memcpy(req, data, length);
		client->CurrentRequest = req;
		SDL_CondSignal(client->WaitFlag);
		#endif
	}
	return -1;
}

/*
 * Acess2 Native Kernel
 * - Acess kernel emulation on another OS using SDL and UDP
 *
 * Syscall Server
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <SDL/SDL.h>
#ifdef __WIN32__
# define _WIN32_WINNT 0x0501
# include <windows.h>
# include <winsock2.h>
# include <ws2tcpip.h>
# define close(fd)	closesocket(fd)
typedef int	socklen_t;
#else
# include <unistd.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <netdb.h>	// getaddrinfo
#endif
#define DONT_INCLUDE_SYSCALL_NAMES
#include "../syscalls.h"
#include <logdebug.h>	// acess but std
#include <errno.h>

#define	USE_TCP	1
#define MAX_CLIENTS	16

// === TYPES ===
typedef struct {
	 int	ClientID;
	SDL_Thread	*WorkerThread;
	tRequestHeader	*CurrentRequest;
	SDL_cond	*WaitFlag;
	SDL_mutex	*Mutex;
	#if USE_TCP
	 int	Socket;
	#else
	struct sockaddr_in	ClientAddr;
	#endif
}	tClient;

// === IMPORTS ===
// TODO: Move these to headers
extern tRequestHeader *SyscallRecieve(tRequestHeader *Request, size_t *ReturnLength);
extern int	Threads_CreateRootProcess(void);
extern void	Threads_SetThread(int TID, void *ClientPtr);
extern void	*Threads_GetThread(int TID);
extern void	Threads_PostEvent(void *Thread, uint32_t Event);
extern void	Threads_int_Terminate(void *Thread);

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
	Uint32	thisId = SDL_ThreadID();
	
	for( int i = 0; i < MAX_CLIENTS; i ++ )
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
	
	// Allocate an ID if needed
	if(ClientID == 0)
		ClientID = Threads_CreateRootProcess();
	
	for( int i = 0; i < MAX_CLIENTS; i ++ )
	{
		if( gaServer_Clients[i].ClientID == ClientID ) {
			return &gaServer_Clients[i];
		}
		if(!ret && gaServer_Clients[i].ClientID == 0)
			ret = &gaServer_Clients[i];
	}
	
	// Uh oh, no free slots
	// TODO: Dynamic allocation
	if( !ret ) {
		Log_Error("Server", "Ran out of static client slots (%i)", MAX_CLIENTS);
		return NULL;
	}
	
	// Allocate a thread for the process
	ret->ClientID = ClientID;
	ret->CurrentRequest = NULL;
	#if USE_TCP
	ret->Socket = 0;
	#endif
		
	if( !ret->WorkerThread ) {
		Log_Debug("Server", "Creating worker for %p", ret);
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

	Log_Debug("Server", "Worker %p active", ClientPtr);	

	tRequestHeader	errorHeader;
	size_t	retSize = 0;
	 int	cur_client_id = 0;
	while( Client->ClientID != 0 )
	{
		// Wait for something to do
		if( Client->CurrentRequest == NULL )
			SDL_CondWait(Client->WaitFlag, Client->Mutex);
		if( Client->CurrentRequest == NULL )
			continue ;
		
		if(Client->ClientID != cur_client_id) {
			Threads_SetThread( Client->ClientID, Client );
			cur_client_id = Client->ClientID;
		}
		
		// Get the response
		tRequestHeader	*retHeader = SyscallRecieve(Client->CurrentRequest, &retSize);

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

		// If the thread is being terminated, don't send reply
		if( Client->ClientID > 0 )
		{
			// Return the data
			#if USE_TCP
			size_t sentSize = send(Client->Socket, retHeader, retSize, 0); 
			#else
			size_t sentSize = sendto(gSocket, retHeader, retSize, 0,
				(struct sockaddr*)&Client->ClientAddr, sizeof(Client->ClientAddr)
				);
			#endif
			if( sentSize != retSize ) {
				perror("Server_WorkerThread - send");
			}
		}
		
		// Free allocated header
		if( retHeader != &errorHeader )
			free( retHeader );
	}
	Log_Notice("Server", "Terminated Worker %p", ClientPtr);	
	return 0;
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
	
	#if USE_TCP
	{
		int val = 1;
		setsockopt(gSocket, SOL_SOCKET, SO_REUSEADDR, &val, sizeof val);
	}
	#endif
	
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

int Server_Shutdown(void)
{
	close(gSocket);
	for( int i = 0; i < MAX_CLIENTS; i ++ )
	{
		if( gaServer_Clients[i].ClientID == 0 )
			continue ;
		Threads_PostEvent( Threads_GetThread(gaServer_Clients[i].ClientID), 0 );
		gaServer_Clients[i].ClientID = -1;
		#if USE_TCP
		close(gaServer_Clients[i].Socket);
		#else
		SDL_CondSignal(gaServer_Clients[i].WaitFlag);
		#endif
	}
	return 0;
}

#if USE_TCP
int Server_int_HandleRx(tClient *Client)
{
	const int	ciMaxParamCount = 6;
	char	lbuf[sizeof(tRequestHeader) + ciMaxParamCount*sizeof(tRequestValue)];
	tRequestHeader	*hdr = (void*)lbuf;
	size_t	len = recv(Client->Socket, (void*)hdr, sizeof(*hdr), 0);
	if( len == 0 ) {
		Log_Notice("Server", "Zero RX on %i (worker %p)", Client->Socket, Client);
		return 1;
	}
	if( len == -1 ) {
		perror("recv header");
		return 2;
	}
	if( len != sizeof(*hdr) ) {
		// Oops?
		Log_Warning("Server", "FD%i bad sized (%i != exp %i)",
			Client->Socket, len, sizeof(*hdr));
		return 0;
	}

	if( hdr->NParams > ciMaxParamCount ) {
		// Oops.
		Log_Warning("Server", "FD%i too many params (%i > max %i)",
			Client->Socket, hdr->NParams, ciMaxParamCount);
		return 0;
	}

	if( hdr->NParams > 0 )
	{
		len = recv(Client->Socket, (void*)hdr->Params, hdr->NParams*sizeof(tRequestValue), 0);
		if( len != hdr->NParams*sizeof(tRequestValue) ) {
			// Oops.
			perror("recv params");
			Log_Warning("Sever", "Recieving params failed");
			return 0;
		}
	}
	else
	{
		//Log_Debug("Server", "No params?");
	}

	// Get buffer size
	size_t	hdrsize = sizeof(tRequestHeader) + hdr->NParams*sizeof(tRequestValue);
	size_t	bufsize = hdrsize;
	for( int i = 0; i < hdr->NParams; i ++ )
	{
		if( hdr->Params[i].Flags & ARG_FLAG_ZEROED )
			;
		else {
			bufsize += hdr->Params[i].Length;
		}
	}

	// Allocate full buffer
	hdr = malloc(bufsize);
	memcpy(hdr, lbuf, hdrsize);
	if( bufsize > hdrsize )
	{
		size_t	rem = bufsize - hdrsize;
		char	*ptr = (void*)( hdr->Params + hdr->NParams );
		while( rem )
		{
			len = recv(Client->Socket, ptr, rem, 0);
			if( len == -1 ) {
				// Oops?
				perror("recv data");
				Log_Warning("Sever", "Recieving data failed");
				return 2;
			}
			rem -= len;
			ptr += len;
		}
		if( rem ) {
			// Extra data?
			return 0;
		}
	}
	else {
		//Log_Debug("Server", "no data");
	}
	
	// Dispatch to worker
	if( Client->CurrentRequest ) {
		printf("Worker thread for client ID %i is busy\n", Client->ClientID);
		return 1;
	}

	// Give to worker
	Log_Debug("Server", "Message from Client %i (%p)", Client->ClientID, Client);
	Client->CurrentRequest = hdr;
	SDL_CondSignal(Client->WaitFlag);

	return 0;
}

int Server_int_HandshakeClient(int Socket, struct sockaddr_in *addr, socklen_t addr_size)
{
	ENTER("iSocket paddr iaddr_size",
		Socket, addr, addr_size);
	unsigned short	port = ntohs(addr->sin_port);
	char	addrstr[4*8+8+1];
	getnameinfo((struct sockaddr*)addr, addr_size, addrstr, sizeof(addrstr), NULL, 0, NI_NUMERICHOST);
	Log_Debug("Server", "Client connection %s:%i", addrstr, port);
	
	// Perform handshake
	tRequestAuthHdr	authhdr;
	size_t	len  = recv(Socket, &authhdr, sizeof(authhdr), 0);
	if( len != sizeof(authhdr) ) {
		// Some form of error?
		Log_Warning("Server", "Client auth block bad size (%i != exp %i)",
			len, sizeof(authhdr));
		LEAVE('i', 1);
		return 1;
	}
	
	LOG("authhdr.pid = %i", authhdr.pid);
	tClient	*client = Server_GetClient(authhdr.pid);
	if( authhdr.pid == 0 ) {
		// Allocate PID and client structure/thread
		client->Socket = Socket;
		authhdr.pid = client->ClientID;
	}
	else {
		Log_Debug("Server", "Client assumed PID %i", authhdr.pid);
		
		// Get client structure and make sure it's unused
		// - Auth token / verifcation?
		if( !client ) {
			Log_Warning("Server", "Can't allocate a client struct for %s:%i",
				addrstr, port);
			LEAVE('i', 1);
			return 1;
		}
		if( client->Socket != 0 ) {
			Log_Warning("Server", "Client (%i)%p owned by FD%i but %s:%i tried to use it",
				authhdr.pid, client, addrstr, port);
			LEAVE('i', 1);
			return 1;
		}
		
		client->Socket = Socket;
	}

	LOG("Sending auth reply");	
	len = send(Socket, (void*)&authhdr, sizeof(authhdr), 0);
	if( len != sizeof(authhdr) ) {
		// Ok, this is an error
		perror("Sending auth reply");
		LEAVE('i', 1);
		return 1;
	}

	// All done, client thread should be watching now		
	
	LEAVE('i', 0);
	return 0;
}

void Server_int_RemoveClient(tClient *Client)
{
	// Trigger the thread to kill itself
	Threads_int_Terminate( Threads_GetThread(Client->ClientID) );
	Client->ClientID = 0;
	close(Client->Socket);
}

#endif

int Server_ListenThread(void *Unused)
{	
	// Wait for something to do :)
	for( ;; )
	{
		#if USE_TCP
		fd_set	fds;
		 int	maxfd = gSocket;
		FD_ZERO(&fds);
		FD_SET(gSocket, &fds);

		for( int i = 0; i < MAX_CLIENTS; i ++ ) {
			tClient	*client = &gaServer_Clients[i];
			if( client->ClientID == 0 )
				continue ;
			FD_SET(client->Socket, &fds);
			if(client->Socket > maxfd)
				maxfd = client->Socket;
		}
		
		int rv = select(maxfd+1, &fds, NULL, NULL, NULL);
		Log_Debug("Server", "Select rv = %i", rv);
		if( rv <= 0 ) {
			perror("select");
			return 1;
		}
		
		// Incoming connection
		if( FD_ISSET(gSocket, &fds) )
		{
			struct sockaddr_in	clientaddr;
			socklen_t	clientSize = sizeof(clientaddr);
			 int	clientSock = accept(gSocket, (struct sockaddr*)&clientaddr, &clientSize);
			if( clientSock < 0 ) {
				perror("SyscallServer - accept");
				break ;
			}
			if( Server_int_HandshakeClient(clientSock, &clientaddr, clientSize) ) {
				Log_Warning("Server", "Client handshake failed :(");
				close(clientSock);
			}
		}
		
		for( int i = 0; i < MAX_CLIENTS; i ++ )
		{
			tClient	*client = &gaServer_Clients[i];
			if( client->ClientID == 0 )
				continue ;
			//Debug("Server_ListenThread: Idx %i ID %i FD %i",
			//	i, client->ClientID, client->Socket);
			if( !FD_ISSET(client->Socket, &fds) )
				continue ;
			
			if( Server_int_HandleRx( client ) )
			{
				Log_Warning("Server", "Client %p dropped, TODO: clean up", client);
				Server_int_RemoveClient(client);
			}
		}
	
		#else
		char	data[BUFSIZ];
		tRequestHeader	*req = (void*)data;
		struct sockaddr_in	addr;
		uint	clientSize = sizeof(addr);
		 int	length;
		
		length = recvfrom(gSocket, data, BUFSIZ, 0, (struct sockaddr*)&addr, &clientSize);
		
		if( length == -1 ) {
			perror("SyscallServer - recv");
			break;
		}
		
		// Recive data
//		Log_Debug("Server", "%i bytes from %x:%i", length,
//			ntohl(addr.sin_addr.s_addr), ntohs(addr.sin_port));
		
		tClient	*client = Server_GetClient(req->ClientID);
		// NOTE: I should really check if the sin_addr is zero, but meh
		// Shouldn't matter much
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
		
//		Log_Debug("AcessSrv", "Message from Client %i (%p)",
//			client->ClientID, client);
		if( client->CurrentRequest ) {
			printf("Worker thread for %x:%i is busy\n",
				ntohl(client->ClientAddr.sin_addr.s_addr), ntohs(client->ClientAddr.sin_port));
			continue;
		}
		
		// Duplicate the data currently on the stack, and dispatch to worker
		req = malloc(length);
		memcpy(req, data, length);
		client->CurrentRequest = req;
		SDL_CondSignal(client->WaitFlag);
		#endif
	}
	return -1;
}

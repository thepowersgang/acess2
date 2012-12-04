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
	#if USE_TCP
	 int	Socket;
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
extern void	*Threads_GetThread(int TID);
extern void	Threads_PostEvent(void *Thread, uint32_t Event);

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
	#if USE_TCP
	ret->Socket = 0;
	#else
	ret->CurrentRequest = NULL;
	#endif
		
	if( !ret->WorkerThread ) {
		#if USE_TCP
		#else
		ret->WaitFlag = SDL_CreateCond();
		ret->Mutex = SDL_CreateMutex();
		SDL_mutexP( ret->Mutex );
		#endif
		Log_Debug("Server", "Creating worker for %p", ret);
		ret->WorkerThread = SDL_CreateThread( Server_WorkerThread, ret );
	}
	
	return ret;
}

int Server_WorkerThread(void *ClientPtr)
{
	tClient	*Client = ClientPtr;

	Log_Debug("Server", "Worker %p", ClientPtr);	

	#if USE_TCP
	while( *((volatile typeof(Client->Socket)*)&Client->Socket) == 0 )
		;
	Threads_SetThread( Client->ClientID );
	
	while( Client->ClientID != -1 )
	{
		fd_set	fds;
		 int	nfd = Client->Socket+1;
		FD_ZERO(&fds);
		FD_SET(Client->Socket, &fds);
		
		int rv = select(nfd, &fds, NULL, NULL, NULL);	// TODO: Timeouts?
		if(rv < 0) {
			perror("select");
			continue ;
		}
//		Log_Debug("Server", "%p: rv=%i", Client, rv);		

		if( FD_ISSET(Client->Socket, &fds) )
		{
			const int	ciMaxParamCount = 6;
			char	lbuf[sizeof(tRequestHeader) + ciMaxParamCount*sizeof(tRequestValue)];
			tRequestHeader	*hdr = (void*)lbuf;
			size_t	len = recv(Client->Socket, (void*)hdr, sizeof(*hdr), 0);
//			Log_Debug("Server", "%i bytes of header", len);
			if( len == 0 ) {
				Log_Notice("Server", "Zero RX on %i (worker %p)", Client->Socket, Client);
				break;
			}
			if( len == -1 ) {
				perror("recv header");
//				Log_Warning("Server", "recv() error - %s", strerror(errno));
				break;
			}
			if( len != sizeof(*hdr) ) {
				// Oops?
				Log_Warning("Server", "FD%i bad sized (%i != exp %i)",
					Client->Socket, len, sizeof(*hdr));
				continue ;
			}

			if( hdr->NParams > ciMaxParamCount ) {
				// Oops.
				Log_Warning("Server", "FD%i too many params (%i > max %i)",
					Client->Socket, hdr->NParams, ciMaxParamCount);
				break ;
			}

			if( hdr->NParams > 0 )
			{
				len = recv(Client->Socket, (void*)hdr->Params, hdr->NParams*sizeof(tRequestValue), 0);
//				Log_Debug("Server", "%i bytes of params", len);
				if( len != hdr->NParams*sizeof(tRequestValue) ) {
					// Oops.
					perror("recv params");
					Log_Warning("Sever", "Recieving params failed");
					break ;
				}
			}
			else
			{
//				Log_Debug("Server", "No params?");
			}

			// Get buffer size
			size_t	hdrsize = sizeof(tRequestHeader) + hdr->NParams*sizeof(tRequestValue);
			size_t	bufsize = hdrsize;
			 int	i;
			for( i = 0; i < hdr->NParams; i ++ )
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
//					Log_Debug("Server", "%i bytes of data", len);
					if( len == -1 ) {
						// Oops?
						perror("recv data");
						Log_Warning("Sever", "Recieving data failed");
						break ;
					}
					rem -= len;
					ptr += len;
				}
				if( rem ) {
					break;
				}
			}
//			else
//				Log_Debug("Server", "no data");

			 int	retlen;
			tRequestHeader	*retHeader;
			retHeader = SyscallRecieve(hdr, &retlen);
			if( !retHeader ) {
				// Some sort of error
				Log_Warning("Server", "SyscallRecieve failed?");
				continue ;
			}
			
			send(Client->Socket, (void*)retHeader, retlen, 0); 

			// Clean up
			free(hdr);
		}
	}
	#else
	tRequestHeader	*retHeader;
	tRequestHeader	errorHeader;
	 int	retSize = 0;
	 int	sentSize;
	 int	cur_client_id = 0;
	while( Client->ClientID != -1 )
	{
		// Wait for something to do
		if( Client->CurrentRequest == NULL )
			SDL_CondWait(Client->WaitFlag, Client->Mutex);
		if( Client->CurrentRequest == NULL )
			continue ;
		
//		Log_Debug("AcessSrv", "Worker got message %p", Client->CurrentRequest);
		
		if(Client->ClientID != cur_client_id) {
//			Log_Debug("AcessSrv", "Client thread ID changed from %i to %i",
//				cur_client_id, Client->ClientID);
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

int Server_ListenThread(void *Unused)
{	
	// Wait for something to do :)
	for( ;; )
	{
		#if USE_TCP
		struct sockaddr_in	clientaddr;
		socklen_t	clientSize = sizeof(clientaddr);
		 int	clientSock = accept(gSocket, (struct sockaddr*)&clientaddr, &clientSize);
		if( clientSock < 0 ) {
			perror("SyscallServer - accept");
			break ;
		}

		char	addrstr[4*8+8+1];
		getnameinfo((struct sockaddr*)&clientaddr, sizeof(clientaddr),
			addrstr, sizeof(addrstr), NULL, 0, NI_NUMERICHOST);
		Log_Debug("Server", "Client connection %s:%i", addrstr, ntohs(clientaddr.sin_port));
		
		// Perform auth
		size_t	len;
		tRequestAuthHdr	authhdr;
		len = recv(clientSock, (void*)&authhdr, sizeof(authhdr), 0);
		if( len != sizeof(authhdr) ) {
			// Some form of error?
			Log_Warning("Server", "Client auth block bad size (%i != exp %i)",
				len, sizeof(authhdr));
			close(clientSock);
			continue ;
		}
		
		Log_Debug("Server", "Client assumed PID %i", authhdr.pid);

		tClient	*client;
		if( authhdr.pid == 0 ) {
			// Allocate PID and client structure/thread
			client = Server_GetClient(0);
			client->Socket = clientSock;
			authhdr.pid = client->ClientID;
		}
		else {
			// Get client structure and make sure it's unused
			// - Auth token / verifcation?
			client = Server_GetClient(authhdr.pid);
			if( !client ) {
				Log_Warning("Server", "Can't allocate a client struct for %s:%i",
					addrstr, clientaddr.sin_port);
				close(clientSock);
				continue ;
			}
			if( client->Socket != 0 ) {
				Log_Warning("Server", "Client (%i)%p owned by FD%i but %s:%i tried to use it",
					authhdr.pid, client, addrstr, clientaddr.sin_port);
				close(clientSock);
				continue;
			}
			else {
				client->Socket = clientSock;
			}
		}
		Log_Debug("Server", "Client given PID %i - info %p", authhdr.pid, client);
		
		len = send(clientSock, (void*)&authhdr, sizeof(authhdr), 0);
		if( len != sizeof(authhdr) ) {
			// Ok, this is an error
			perror("Sending auth reply");
		}

		// All done, client thread should be watching now		

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
		
//		Log_Debug("AcessSrv", "Message from Client %i (%p)",
//			client->ClientID, client);

		// Make a copy of the request data	
		req = malloc(length);
		memcpy(req, data, length);
		client->CurrentRequest = req;
		SDL_CondSignal(client->WaitFlag);
		#endif
	}
	return -1;
}

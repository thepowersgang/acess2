/*
 * Acess2 Kernel
 * - By John Hodge (thePowersGang);
 * 
 * drv/dgram_pipe.c
 * - Connection+Datagram based local IPC
 */
#include <vfs.h>
#include <fs_devfs.h>
#include <modules.h>
#include <rwlock.h>
#include <semaphore.h>

#define DEF_MAX_BLOCK_SIZE	0x1000
#define DEF_MAX_BYTE_LIMIT	0x8000

// === TYPES ===
typedef struct sIPCPipe_Packet	tIPCPipe_Packet;
typedef struct sIPCPipe_Endpoint	tIPCPipe_Endpoint;
typedef struct sIPCPipe_Channel	tIPCPipe_Channel;
typedef struct sIPCPipe_Server	tIPCPipe_Server;

// === STRUCTURES ===
struct sIPCPipe_Packet
{
	tIPCPipe_Packet	*Next;
	size_t	Length;
	size_t	Offset;	//!< Offset to first unread byte (for streams)
	void	*Data;
};
struct sIPCPipe_Endpoint
{
	tMutex	lList;
	tIPCPipe_Packet	*OutHead;
	tIPCPipe_Packet	*OutTail;
	tSemaphore	InCount;
	tVFS_Node	Node;
};
struct sIPCPipe_Channel
{
	tIPCPipe_Channel	*Next;
	tIPCPipe_Channel	*Prev;
	tIPCPipe_Server	*Server;
	tIPCPipe_Endpoint	ServerEP;
	tIPCPipe_Endpoint	ClientEP;
};
struct sIPCPipe_Server
{
	tIPCPipe_Server	*Next;
	tIPCPipe_Server	*Prev;
	char	*Name;
	size_t	MaxBlockSize;	// Max size of a 'packet'
	size_t	QueueByteLimit;	// Maximum number of bytes held in kernel for this server
	size_t	CurrentByteCount;
	tVFS_Node	ServerNode;
	tSemaphore	ConnectionSemaphore;
	tRWLock	lChannelList;
	tIPCPipe_Channel	*FirstClient;
	tIPCPipe_Channel	*LastClient;
	tIPCPipe_Channel	*FirstUnseenClient;	// !< First client server hasn't opened
};

// === PROTOTYPES ===
 int	IPCPipe_Install(char **Arguments);
// - Root
tVFS_Node	*IPCPipe_Root_MkNod(tVFS_Node *Node, const char *Name, Uint Flags);
 int	IPCPipe_Root_ReadDir(tVFS_Node *Node, int ID, char Name[FILENAME_MAX]);
tVFS_Node	*IPCPipe_Root_FindDir(tVFS_Node *Node, const char *Name);
// - Server
 int	IPCPipe_Server_ReadDir(tVFS_Node *Node, int ID, char Name[FILENAME_MAX]);
tVFS_Node	*IPCPipe_Server_FindDir(tVFS_Node *Node, const char *Name);
void	IPCPipe_Server_Close(tVFS_Node *Node);
// - Socket
tIPCPipe_Channel	*IPCPipe_int_GetEPs(tVFS_Node *Node, tIPCPipe_Endpoint **lep, tIPCPipe_Endpoint **rep);
size_t	IPCPipe_Client_Read(tVFS_Node *Node, off_t Offset, size_t Length, void *Dest);
size_t	IPCPipe_Client_Write(tVFS_Node *Node, off_t Offset, size_t Length, const void *Src);
void	IPCPipe_Client_Close(tVFS_Node *Node);

// === GLOBALS ===
// - Server node: Directory
tVFS_NodeType	gIPCPipe_ServerNodeType = {
	.TypeName = "IPC Pipe - Server",
	.ReadDir = IPCPipe_Server_ReadDir,
	.FindDir = IPCPipe_Server_FindDir,
	.Close = IPCPipe_Server_Close
};
tVFS_NodeType	gIPCPipe_ChannelNodeType = {
	.TypeName = "IPC Pipe - Channel",
	.Read = IPCPipe_Client_Read,
	.Write = IPCPipe_Client_Write,
	.Close = IPCPipe_Client_Close
};
tVFS_NodeType	gIPCPipe_RootNodeType = {
	.TypeName = "IPC Pipe - Root",
	.MkNod = IPCPipe_Root_MkNod,
	.ReadDir = IPCPipe_Root_ReadDir,
	.FindDir = IPCPipe_Root_FindDir
};
tDevFS_Driver	gIPCPipe_DevFSInfo = {
	.Name = "ipcpipe",
	.RootNode = {.Type=&gIPCPipe_RootNodeType}
};
// - Global list of servers
tRWLock	glIPCPipe_ServerList;
tIPCPipe_Server	*gpIPCPipe_FirstServer;
tIPCPipe_Server	*gpIPCPipe_LastServer;
// - Module definition
MODULE_DEFINE(0, 0x0100, IPCPipe, IPCPipe_Install, NULL, NULL);

// === CODE ===
int IPCPipe_Install(char **Arguments)
{
	DevFS_AddDevice(&gIPCPipe_DevFSInfo);
	return MODULE_ERR_OK;
}

// - Root -
/*
 * \brief Create a new named pipe
 * \note Expected to be called from VFS_Open with OPENFLAG_CREATE
 */
tVFS_Node *IPCPipe_Root_MkNod(tVFS_Node *Node, const char *Name, Uint Flags)
{
	tIPCPipe_Server	*srv;
	
	// Write Lock
	RWLock_AcquireWrite(&glIPCPipe_ServerList);
	for( srv = gpIPCPipe_FirstServer; srv; srv = srv->Next )
	{
		if( strcmp(srv->Name, Name) == 0 )
			break;
	}
	if( srv )
	{
		RWLock_Release(&glIPCPipe_ServerList);
		return NULL;
	}
	

	srv = calloc( 1, sizeof(tIPCPipe_Server) + strlen(Name) + 1 );
	srv->Name = (void*)(srv + 1);
	strcpy(srv->Name, Name);
	srv->MaxBlockSize = DEF_MAX_BLOCK_SIZE;
	srv->QueueByteLimit = DEF_MAX_BYTE_LIMIT;
	srv->ServerNode.Type = &gIPCPipe_ServerNodeType;
	srv->ServerNode.ImplPtr = srv;

	// Lock already held
	if( gpIPCPipe_LastServer )
		gpIPCPipe_LastServer->Next = srv;
	else
		gpIPCPipe_FirstServer = srv;
	srv->Prev = gpIPCPipe_LastServer;
	gpIPCPipe_LastServer = srv;
	RWLock_Release(&glIPCPipe_ServerList);

	return &srv->ServerNode;
}

int IPCPipe_Root_ReadDir(tVFS_Node *Node, int ID, char Name[FILENAME_MAX])
{
	tIPCPipe_Server	*srv;
	RWLock_AcquireRead(&glIPCPipe_ServerList);
	for( srv = gpIPCPipe_FirstServer; srv && ID--; srv = srv->Next )
		;
	RWLock_Release(&glIPCPipe_ServerList);
	
	if( srv ) {
		strncpy(Name, srv->Name, sizeof(Name));
		return 0;
	}
	
	return -1;
}
/**
 * \return New client pointer
 */
tVFS_Node *IPCPipe_Root_FindDir(tVFS_Node *Node, const char *Name)
{
	tIPCPipe_Server	*srv;
	
	// Find server
	RWLock_AcquireRead(&glIPCPipe_ServerList);
	for( srv = gpIPCPipe_FirstServer; srv; srv = srv->Next )
	{
		if( strcmp(srv->Name, Name) == 0 )
			break;
	}
	RWLock_Release(&glIPCPipe_ServerList);
	if( !srv )
		return NULL;

	// Create new client
	tIPCPipe_Channel *new_client;
	
	new_client = calloc(1, sizeof(tIPCPipe_Channel));
	new_client->Server = srv;
	new_client->ClientEP.Node.Type = &gIPCPipe_ChannelNodeType;
	new_client->ClientEP.Node.ImplPtr = new_client;
	new_client->ServerEP.Node.Type = &gIPCPipe_ChannelNodeType;
	new_client->ServerEP.Node.ImplPtr = new_client;

	// Append to server list
	RWLock_AcquireWrite(&srv->lChannelList);
	if(srv->LastClient)
		srv->LastClient->Next = new_client;
	else
		srv->FirstClient = new_client;
	srv->LastClient = new_client;
	if(!srv->FirstUnseenClient)
		srv->FirstUnseenClient = new_client;
	RWLock_Release(&srv->lChannelList);
	
	return &new_client->ClientEP.Node;
}

// --- Server ---
int IPCPipe_Server_ReadDir(tVFS_Node *Node, int ID, char Name[FILENAME_MAX])
{
	// 'next' is a valid entry, but readdir should never be called on this node
	return -1;
}
tVFS_Node *IPCPipe_Server_FindDir(tVFS_Node *Node, const char *Name)
{
	tIPCPipe_Server	*srv = Node->ImplPtr;
	
	if( strcmp(Name, "next") != 0 )
		return NULL;
	if( Semaphore_Wait( &srv->ConnectionSemaphore, 1 ) != 1 )
		return NULL;

	tIPCPipe_Channel *conn;
	RWLock_AcquireRead(&srv->lChannelList);
	conn = srv->FirstUnseenClient;
	if( conn )
	{
		srv->FirstUnseenClient = conn->Next;
	}
	RWLock_Release(&srv->lChannelList);

	if( !conn )
		return NULL;
	
	// Success
	return &conn->ServerEP.Node;
}

void IPCPipe_Server_Close(tVFS_Node *Node)
{
	tIPCPipe_Server	*srv = Node->ImplPtr;
//	Semaphore_Cancel(&srv->ConnectionSemaphore);

	// Flag server as being destroyed

	// Force-close all children
	RWLock_AcquireWrite(&srv->lChannelList);
	for(tIPCPipe_Channel *client = srv->FirstClient; client; client = client->Next)
	{
		client->Server = NULL;
	}
	RWLock_Release(&srv->lChannelList);

	// Remove from global list
	RWLock_AcquireWrite(&glIPCPipe_ServerList);
	// - Forward link
	if(srv->Prev)
		srv->Prev->Next = srv->Next;
	else
		gpIPCPipe_FirstServer = srv->Next;
	// - Reverse link
	if(srv->Next)
		srv->Next->Prev = srv->Prev;
	else
		gpIPCPipe_LastServer = srv->Prev;
	RWLock_Release(&glIPCPipe_ServerList);
}

// --- Channel ---
tIPCPipe_Channel *IPCPipe_int_GetEPs(tVFS_Node *Node, tIPCPipe_Endpoint **lep, tIPCPipe_Endpoint **rep)
{
	tIPCPipe_Channel	*ch = Node->ImplPtr;
	if( !ch )	return NULL;
	*lep = (Node == &ch->ServerEP.Node ? &ch->ServerEP : &ch->ClientEP);
	*rep = (Node == &ch->ServerEP.Node ? &ch->ClientEP : &ch->ServerEP);
	return ch;
}
size_t IPCPipe_Client_Read(tVFS_Node *Node, off_t Offset, size_t Length, void *Dest)
{
	tIPCPipe_Endpoint	*lep, *rep;
	tIPCPipe_Channel	*channel = IPCPipe_int_GetEPs(Node, &lep, &rep);
	
	if( channel->Server == NULL )
		return -1;
	
	if( Semaphore_Wait(&lep->InCount, 1) != 1 )
	{
		if( channel->Server == NULL )
			return -1;
		return 0;
	}

	Mutex_Acquire(&rep->lList);
	tIPCPipe_Packet	*pkt = rep->OutHead;
	if(pkt)
		rep->OutHead = pkt->Next;
	if(!rep->OutHead)
		rep->OutTail = NULL;
	Mutex_Release(&rep->lList);

	if(!pkt)
		return 0;

	size_t ret = MIN(pkt->Length, Length);
	memcpy(Dest, pkt->Data, ret);
	free(pkt);
	
	return ret;
}

size_t IPCPipe_Client_Write(tVFS_Node *Node, off_t Offset, size_t Length, const void *Src)
{
	tIPCPipe_Endpoint	*lep, *rep;
	tIPCPipe_Channel	*channel = IPCPipe_int_GetEPs(Node, &lep, &rep);
	
	// Ensure the server hasn't been closed
	if( !channel || channel->Server == NULL )
		return -1;

	if( Length > channel->Server->MaxBlockSize )
		return 0;

	// Create packet structure	
	tIPCPipe_Packet	*pkt = malloc(sizeof(tIPCPipe_Packet)+Length);
	pkt->Next = NULL;
	pkt->Offset = 0;
	pkt->Length = Length;
	pkt->Data = pkt + 1;
	memcpy(pkt->Data, Src, Length);

	// Append to list
	Mutex_Acquire(&lep->lList);
	if( !Node->ImplPtr )
	{
		// Client was closed by another thread
		free(pkt);
		Mutex_Release(&lep->lList);
		return -1;
	}
	if(lep->OutTail)
		lep->OutTail->Next = pkt;
	else
		lep->OutHead = pkt;
	lep->OutTail = pkt;
	Mutex_Release(&lep->lList);

	// Signal other end
	Semaphore_Signal(&rep->InCount, 1);

	return Length;
}

void IPCPipe_Client_Close(tVFS_Node *Node)
{
	tIPCPipe_Endpoint	*lep, *rep;
	tIPCPipe_Channel	*ch = IPCPipe_int_GetEPs(Node, &lep, &rep);
	
	// Mark client as closed
	Node->ImplPtr = NULL;
	Mutex_Acquire(&lep->lList);
	while( lep->OutHead ) {
		tIPCPipe_Packet	*next = lep->OutHead->Next;
		free(lep->OutHead);
		lep->OutHead = next;
	}
	Mutex_Release(&lep->lList);
	
	// Clean up if both sides are closed
	if( rep->Node.ImplPtr == NULL )
	{
		tIPCPipe_Server	*srv = ch->Server;
		if( srv )
		{
			RWLock_AcquireWrite(&srv->lChannelList);
			if(ch->Prev)
				ch->Prev->Next = ch->Next;
			else
				srv->FirstClient = ch->Next;
			if(ch->Next)
				ch->Next->Prev = ch->Prev;
			else
				srv->LastClient = ch->Prev;
			if(srv->FirstUnseenClient == ch)
				srv->FirstUnseenClient = ch->Next;
			RWLock_Release(&srv->lChannelList);
		}
		free(ch);
	}
}

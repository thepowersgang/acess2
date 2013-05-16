/* 
 * Acess2 Kernel
 * - By John Hodge (thePowersGang)
 *
 * drv/fifo.c
 * - FIFO Pipe Driver
 */
#define DEBUG	0
#include <acess.h>
#include <modules.h>
#include <fs_devfs.h>
#include <semaphore.h>

// === CONSTANTS ===
#define DEFAULT_RING_SIZE	2048
#define PF_BLOCKING		1

// === TYPES ===
typedef struct sPipe {
	struct sPipe	*Next;
	char	*Name;
	tVFS_Node	Node;
	Uint	Flags;
	 int	ReadPos;
	 int	WritePos;
	 int	BufSize;
	char	*Buffer;
} tPipe;

// === PROTOTYPES ===
 int	FIFO_Install(char **Arguments);
 int	FIFO_IOCtl(tVFS_Node *Node, int Id, void *Data);
 int	FIFO_ReadDir(tVFS_Node *Node, int Id, char Dest[FILENAME_MAX]);
tVFS_Node	*FIFO_FindDir(tVFS_Node *Node, const char *Filename, Uint Flags);
tVFS_Node	*FIFO_MkNod(tVFS_Node *Node, const char *Name, Uint Flags);
void	FIFO_Reference(tVFS_Node *Node);
void	FIFO_Close(tVFS_Node *Node);
 int	FIFO_Unlink(tVFS_Node *Node, const char *OldName);
size_t	FIFO_Read(tVFS_Node *Node, off_t Offset, size_t Length, void *Buffer, Uint Flags);
size_t	FIFO_Write(tVFS_Node *Node, off_t Offset, size_t Length, const void *Buffer, Uint Flags);
tPipe	*FIFO_Int_NewPipe(int Size, const char *Name);

// === GLOBALS ===
MODULE_DEFINE(0, 0x0032, FIFO, FIFO_Install, NULL, NULL);
tVFS_NodeType	gFIFO_DirNodeType = {
	.TypeName = "FIFO Dir Node",
	.ReadDir = FIFO_ReadDir,
	.FindDir = FIFO_FindDir,
	.MkNod = FIFO_MkNod,
	.Unlink = FIFO_Unlink,
	.IOCtl = FIFO_IOCtl
};
tVFS_NodeType	gFIFO_PipeNodeType = {
	.TypeName = "FIFO Pipe Node",
	.Read = FIFO_Read,
	.Write = FIFO_Write,
	.Close = FIFO_Close,
	.Reference = FIFO_Reference
};
tDevFS_Driver	gFIFO_DriverInfo = {
	NULL, "fifo",
	{
	.Size = 1,
	.NumACLs = 1,
	.ACLs = &gVFS_ACL_EveryoneRW,
	.Flags = VFS_FFLAG_DIRECTORY,
	.Type = &gFIFO_DirNodeType
	}
};
tVFS_Node	gFIFO_AnonNode = {
	.NumACLs = 1,
	.ACLs = &gVFS_ACL_EveryoneRW,
	};
tPipe	*gFIFO_NamedPipes = NULL;

// === CODE ===
/**
 * \fn int FIFO_Install(char **Options)
 * \brief Installs the FIFO Driver
 */
int FIFO_Install(char **Options)
{
	DevFS_AddDevice( &gFIFO_DriverInfo );
	return MODULE_ERR_OK;
}

/**
 * \fn int FIFO_IOCtl(tVFS_Node *Node, int Id, void *Data)
 */
int FIFO_IOCtl(tVFS_Node *Node, int Id, void *Data)
{
	return 0;
}

/**
 * \fn char *FIFO_ReadDir(tVFS_Node *Node, int Id)
 * \brief Reads from the FIFO root
 */
int FIFO_ReadDir(tVFS_Node *Node, int Id, char Dest[FILENAME_MAX])
{
	tPipe	*tmp = gFIFO_NamedPipes;
	
	// Entry 0 is Anon Pipes
	if(Id == 0) {
		strcpy(Dest, "anon");
		return 0;
	}
	
	// Find the id'th node
	while(--Id && tmp)	tmp = tmp->Next;
	// If the list ended, error return
	if(!tmp)
		return -EINVAL;
	// Return good
	strncpy(Dest, tmp->Name, FILENAME_MAX);
	return 0;
}

/**
 * \fn tVFS_Node *FIFO_FindDir(tVFS_Node *Node, const char *Filename)
 * \brief Find a file in the FIFO root
 * \note Creates an anon pipe if anon is requested
 */
tVFS_Node *FIFO_FindDir(tVFS_Node *Node, const char *Filename, Uint Flags)
{
	tPipe	*tmp;
	if(!Filename)	return NULL;
	
	// NULL String Check
	if(Filename[0] == '\0')	return NULL;
	
	// Anon Pipe
	if( strcmp(Filename, "anon") == 0 )
	{
		if( Flags & VFS_FDIRFLAG_STAT ) {
			//return &gFIFI_TemplateAnonNode;
		}
		tmp = FIFO_Int_NewPipe(DEFAULT_RING_SIZE, "anon");
		return &tmp->Node;
	}
	
	// Check Named List
	tmp = gFIFO_NamedPipes;
	while(tmp)
	{
		if(strcmp(tmp->Name, Filename) == 0)
			return &tmp->Node;
		tmp = tmp->Next;
	}
	return NULL;
}

/**
 * \fn int FIFO_MkNod(tVFS_Node *Node, const char *Name, Uint Flags)
 */
tVFS_Node *FIFO_MkNod(tVFS_Node *Node, const char *Name, Uint Flags)
{
	return 0;
}

void FIFO_Reference(tVFS_Node *Node)
{
	if(!Node->ImplPtr)	return ;
	
	Node->ReferenceCount ++;
}

/**
 * \fn void FIFO_Close(tVFS_Node *Node)
 * \brief Close a FIFO end
 */
void FIFO_Close(tVFS_Node *Node)
{
	tPipe	*pipe;
	if(!Node->ImplPtr)	return ;
	
	Node->ReferenceCount --;
	if(Node->ReferenceCount)	return ;
	
	pipe = Node->ImplPtr;
	
	if(strcmp(pipe->Name, "anon") == 0) {
		Log_Debug("FIFO", "Pipe %p closed", Node->ImplPtr);
		free(Node->ImplPtr);
		return ;
	}
	
	return ;
}

/**
 * \brief Delete a pipe
 */
int FIFO_Unlink(tVFS_Node *Node, const char *OldName)
{
	tPipe	*pipe;
	
	if(Node != &gFIFO_DriverInfo.RootNode)	return 0;
	
	// Can't relink anon
	if(strcmp(OldName, "anon"))	return 0;
	
	// Find node
	for(pipe = gFIFO_NamedPipes;
		pipe;
		pipe = pipe->Next)
	{
		if(strcmp(pipe->Name, OldName) == 0)
			break;
	}
	if(!pipe)	return 0;
	
	// Unlink the pipe
	if(Node->ImplPtr) {
		free(Node->ImplPtr);
		return 1;
	}
	
	return 0;
}

/**
 * \brief Read from a fifo pipe
 */
size_t FIFO_Read(tVFS_Node *Node, off_t Offset, size_t Length, void *Buffer, Uint Flags)
{
	tPipe	*pipe = Node->ImplPtr;
	Uint	len;
	Uint	remaining = Length;

	if(!pipe)	return 0;
	
	ENTER("pNode XOffset xLength pBuffer", Node, Offset, Length, Buffer);
	
	while(remaining)
	{
		// Wait for buffer to fill
		if( (pipe->Flags & PF_BLOCKING) && !(Flags & VFS_IOFLAG_NOBLOCK) )
		{
			if( pipe->ReadPos == pipe->WritePos )
				VFS_SelectNode(Node, VFS_SELECT_READ, NULL, "FIFO_Read");
			
		}
		else
		{
			if(pipe->ReadPos == pipe->WritePos)
			{
				VFS_MarkAvaliable(Node, 0);
				LEAVE('i', 0);
				return 0;
			}
		}
	
		len = remaining;
		if( pipe->ReadPos < pipe->WritePos )
		{
			 int	avail_bytes = pipe->WritePos - pipe->ReadPos;
			if( avail_bytes < remaining )	len = avail_bytes;
		}
		else
		{
			 int	avail_bytes = pipe->WritePos + pipe->BufSize - pipe->ReadPos;
			if( avail_bytes < remaining )	len = avail_bytes;
		}

		LOG("len = %i, remaining = %i", len, remaining);		

		// Check if read overflows buffer
		if(len > pipe->BufSize - pipe->ReadPos)
		{
			int ofs = pipe->BufSize - pipe->ReadPos;
			memcpy(Buffer, &pipe->Buffer[pipe->ReadPos], ofs);
			memcpy((Uint8*)Buffer + ofs, &pipe->Buffer, len-ofs);
		}
		else
		{
			memcpy(Buffer, &pipe->Buffer[pipe->ReadPos], len);
		}
		
		// Increment read position
		pipe->ReadPos += len;
		pipe->ReadPos %= pipe->BufSize;
		
		// Mark some flags
		if( pipe->ReadPos == pipe->WritePos ) {
			LOG("%i == %i, marking none to read", pipe->ReadPos, pipe->WritePos);
			VFS_MarkAvaliable(Node, 0);
		}
		VFS_MarkFull(Node, 0);	// Buffer can't still be full
		
		// Decrement Remaining Bytes
		remaining -= len;
		// Increment Buffer address
		Buffer = (Uint8*)Buffer + len;
		
		// TODO: Option to read differently
		LEAVE('i', len);
		return len;
	}

	LEAVE('i', Length);
	return Length;

}

/**
 * \brief Write to a fifo pipe
 */
size_t FIFO_Write(tVFS_Node *Node, off_t Offset, size_t Length, const void *Buffer, Uint Flags)
{
	tPipe	*pipe = Node->ImplPtr;
	Uint	len;
	Uint	remaining = Length;
	
	if(!pipe)	return 0;

	ENTER("pNode XOffset xLength pBuffer", Node, Offset, Length, Buffer);
	
	while(remaining)
	{
		// Wait for buffer to empty
		if( (pipe->Flags & PF_BLOCKING) && !(Flags & VFS_IOFLAG_NOBLOCK) )
		{
			if( pipe->ReadPos == (pipe->WritePos+1)%pipe->BufSize ) {
				LOG("Blocking write on FIFO");
				VFS_SelectNode(Node, VFS_SELECT_WRITE, NULL, "FIFO_Write");
			}

			len = remaining;
			if( pipe->ReadPos > pipe->WritePos )
			{
				 int	rem_space = pipe->ReadPos - pipe->WritePos;
				if(rem_space < remaining)	len = rem_space;
			}
			else
			{
				 int	rem_space = pipe->ReadPos + pipe->BufSize - pipe->WritePos;
				if(rem_space < remaining)	len = rem_space;
			}
		}
		else
		{
			if(pipe->ReadPos == (pipe->WritePos+1)%pipe->BufSize)
			{
				LEAVE('i', 0);
				return 0;
			}
			// Write buffer
			if(pipe->ReadPos - pipe->WritePos < remaining)
				len = pipe->ReadPos - pipe->WritePos;
			else
				len = remaining;
		}
		
		// Check if write overflows buffer
		if(len > pipe->BufSize - pipe->WritePos)
		{
			int ofs = pipe->BufSize - pipe->WritePos;
			LOG("pipe->Buffer = %p, pipe->WritePos = %i, ofs=%i, len=%i",
				pipe->Buffer, pipe->WritePos, ofs, len);
			memcpy(&pipe->Buffer[pipe->WritePos], Buffer, ofs);
			memcpy(&pipe->Buffer[0], (Uint8*)Buffer + ofs, len-ofs);
		}
		else
		{
			LOG("pipe->Buffer = %p, pipe->WritePos = %i", pipe->Buffer, pipe->WritePos);
			memcpy(&pipe->Buffer[pipe->WritePos], Buffer, len);
		}
		
		// Increment read position
		pipe->WritePos += len;
		pipe->WritePos %= pipe->BufSize;
		
		// Mark some flags
		if( pipe->ReadPos == pipe->WritePos ) {
			LOG("Buffer is full");
			VFS_MarkFull(Node, 1);	// Buffer full
		}
		VFS_MarkAvaliable(Node, 1);
		
		// Decrement Remaining Bytes
		remaining -= len;
		// Increment Buffer address
		Buffer = (Uint8*)Buffer + len;
	}

	LEAVE('i', Length);
	return Length;
}

// --- HeLPERS ---
/**
 * \fn tPipe *FIFO_Int_NewPipe(int Size, const char *Name)
 * \brief Create a new pipe
 */
tPipe *FIFO_Int_NewPipe(int Size, const char *Name)
{
	tPipe	*ret;
	 int	namelen = strlen(Name) + 1;
	 int	allocsize = sizeof(tPipe) + sizeof(tVFS_ACL) + Size + namelen;

	ENTER("iSize sName", Size, Name);	

	ret = calloc(1, allocsize);
	if(!ret)	LEAVE_RET('n', NULL);
	
	// Set default flags
	ret->Flags = PF_BLOCKING;
	
	// Allocate Buffer
	ret->BufSize = Size;
	ret->Buffer = (void*)( (Uint)ret + sizeof(tPipe) + sizeof(tVFS_ACL) );
	
	// Set name (and FIFO name)
	ret->Name = ret->Buffer + Size;
	strcpy(ret->Name, Name);
	// - Start empty, max of `Size`
	//Semaphore_Init( &ret->Semaphore, 0, Size, "FIFO", ret->Name );
	
	// Set Node
	ret->Node.ReferenceCount = 1;
	ret->Node.Size = 0;
	ret->Node.ImplPtr = ret;
	ret->Node.UID = Threads_GetUID();
	ret->Node.GID = Threads_GetGID();
	ret->Node.NumACLs = 1;
	ret->Node.ACLs = (void*)( (Uint)ret + sizeof(tPipe) );
		ret->Node.ACLs->Ent.Group = 0;
		ret->Node.ACLs->Ent.ID = ret->Node.UID;
		ret->Node.ACLs->Perm.Inv = 0;
		ret->Node.ACLs->Perm.Perms = -1;
	ret->Node.CTime
		= ret->Node.MTime
		= ret->Node.ATime = now();
	ret->Node.Type = &gFIFO_PipeNodeType;

	LEAVE('p', ret);
	
	return ret;
}

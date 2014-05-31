/*
 * Acess2 Kernel
 * - By John Hodge (thePowersGang)
 *
 * drv/pty.c
 * - Pseudo Terminals
 */
#define DEBUG	0
#include <acess.h>
#include <vfs.h>
#include <fs_devfs.h>
#include <drv_pty.h>
#include <modules.h>
#include <rwlock.h>
#include <mutex.h>
#include <posix_signals.h>

// === CONSTANTS ===
#define OUTPUT_RINGBUFFER_LEN	1024	// Number of bytes in output queue before client blocks
#define INPUT_RINGBUFFER_LEN	256	// Number of bytes in input queue before being dropped
#define INPUT_LINE_LEN	256

// === TYPES ===
struct sPTY
{
	tPTY	*Next;
	
	char	*Name;
	 int	NumericName;
	
	void	*OutputHandle;
	tPTY_OutputFcn	OutputFcn;
	tPTY_ReqResize	ReqResize;
	tPTY_ModeSet	ModeSet;

	struct ptymode	Mode;
	struct ptydims	Dims;

	 int	HasHitEOF;	
	tMutex	InputMutex;
	 int	InputWritePos;
	 int	InputReadPos;
	char	InputData[INPUT_RINGBUFFER_LEN];
	
	 int	LineLength;
	char	LineData[INPUT_LINE_LEN];

	tMutex	OutputMutex;	
	 int	OutputWritePos;
	 int	OutputReadPos;
	char	OutputData[OUTPUT_RINGBUFFER_LEN];
	
	tVFS_Node	*ServerNode;
	tVFS_Node	ClientNode;
	tVFS_ACL	OwnerRW;

	tPGID	ControllingProcGroup;
};

// === PROTOTYPES ===
 int	PTY_Install(char **Arguments);
 int	PTY_ReadDir(tVFS_Node *Node, int Pos, char Name[FILENAME_MAX]);
tVFS_Node	*PTY_FindDir(tVFS_Node *Node, const char *Name, Uint Flags);

size_t	_rb_write(void *buf, size_t buflen, int *rd, int *wr, const void *data, size_t len);
size_t	_rb_read(void *buf, size_t buflen, int *rd, int *wr, void *data, size_t len);
size_t	PTY_int_WriteInput(tPTY *PTY, const char *Input, size_t Length);
size_t	PTY_int_SendInput(tPTY *PTY, const char *Input, size_t Length);
// PTY_SendInput
size_t	PTY_ReadClient(tVFS_Node *Node, off_t Offset, size_t Length, void *Buffer, Uint Flags);
size_t	PTY_WriteClient(tVFS_Node *Node, off_t Offset, size_t Length, const void *Buffer, Uint Flags);
void	PTY_ReferenceClient(tVFS_Node *Node);
void	PTY_CloseClient(tVFS_Node *Node);
size_t	PTY_ReadServer(tVFS_Node *Node, off_t Offset, size_t Length, void *Buffer, Uint Flags);
size_t	PTY_WriteServer(tVFS_Node *Node, off_t Offset, size_t Length, const void *Buffer, Uint Flags);
void	PTY_CloseServer(tVFS_Node *Node);
 int	PTY_IOCtl(tVFS_Node *Node, int ID, void *Arg);

// === GLOBALS ===
MODULE_DEFINE(0, 0x100, PTY, PTY_Install, NULL, NULL);
tVFS_NodeType	gPTY_NodeType_Root = {
	.TypeName = "PTY-Root",
	.ReadDir = PTY_ReadDir,
	.FindDir = PTY_FindDir,
};
tVFS_NodeType	gPTY_NodeType_Client = {
	.TypeName = "PTY-Client",
	.Read = PTY_ReadClient,
	.Write = PTY_WriteClient,
	.IOCtl = PTY_IOCtl,
	.Reference = PTY_ReferenceClient,
	.Close = PTY_CloseClient
};
tVFS_NodeType	gPTY_NodeType_Server = {
	.TypeName = "PTY-Server",
	.Read = PTY_ReadServer,
	.Write = PTY_WriteServer,
	.IOCtl = PTY_IOCtl,
	.Close = PTY_CloseServer
};
tDevFS_Driver	gPTY_Driver = {
	.Name = "pts",
	.RootNode = {
		.Flags = VFS_FFLAG_DIRECTORY,
		.Type = &gPTY_NodeType_Root,
		.Size = -1
	}
};
 int	giPTY_NumCount;
tRWLock	glPTY_NumPTYs;
tPTY	*gpPTY_FirstNumPTY;
 int	giPTY_NamedCount;
tRWLock	glPTY_NamedPTYs;
tPTY	*gpPTY_FirstNamedPTY;

// === CODE ===
int PTY_Install(char **Arguments)
{
	DevFS_AddDevice(&gPTY_Driver);
	return MODULE_ERR_OK;
}

// --- Management ---
tPTY *PTY_Create(const char *Name, void *Handle, tPTY_OutputFcn Output, tPTY_ReqResize ReqResize, tPTY_ModeSet ModeSet, const struct ptydims *InitialDims, const struct ptymode *InitialMode)
{
	tPTY	**prev_np = NULL;
	size_t	namelen;
	 int	idx = 1;
	
	if( !Name )
		Name = "";
	
	if( Name[0] == '\0' )
	{
		RWLock_AcquireWrite(&glPTY_NumPTYs);
		// Get a pty ID if Name==NULL
		prev_np = &gpPTY_FirstNumPTY;
		for( tPTY *pty = gpPTY_FirstNumPTY; pty; prev_np = &pty->Next, pty = pty->Next )
		{
			if( pty->NumericName > idx )
				break;
			idx ++;
		}
		namelen = snprintf(NULL,0, "%u", idx);
	}
	else if( Name[strlen(Name)-1] == '#' )
	{
		// Sequenced PTYs
		// - "gui#" would translate to "gui0", "gui1", "gui2", ...
		//   whichever is free
		prev_np = &gpPTY_FirstNamedPTY;

		RWLock_AcquireWrite(&glPTY_NamedPTYs);
		idx = 0;
		namelen = strlen(Name)-1;
		for( tPTY *pty = gpPTY_FirstNamedPTY; pty; prev_np = &pty->Next, pty = pty->Next )
		{
			 int	cmp = strncmp(pty->Name, Name, namelen);
			if( cmp < 0 )
				continue ;
			if( cmp > 0 )
				break;

			// Skip non-numbered
			if( pty->Name[namelen] == '\0' )
				continue ;			

			// Find an unused index
			char	*name_end;
			 int	this_idx = strtol(pty->Name+namelen, &name_end, 10);
			if( *name_end != '\0' )
				continue;
			if( this_idx > idx )
				break;
			idx ++;
		}
		
		namelen += snprintf(NULL, 0, "%u", idx);
	}
	else
	{
		prev_np = &gpPTY_FirstNamedPTY;
		
		// Check the name isn't decimal
		char *end;
		if( strtol(Name, &end, 10) != 0 && *end == '\0' ) {
			errno = EINVAL;
			return NULL;
		}

		RWLock_AcquireWrite(&glPTY_NamedPTYs);
		// Detect duplicates
		for( tPTY *pty = gpPTY_FirstNamedPTY; pty; prev_np = &pty->Next, pty = pty->Next )
		{
			 int	cmp = strcmp(pty->Name, Name);
			if( cmp < 0 )
				continue;
			if( cmp == 0 ) {
				RWLock_Release(&glPTY_NamedPTYs);
				errno = EEXIST;
				return NULL;
			}
			break;
		}
		namelen = strlen(Name);
		idx = -1;
	}
	
	tPTY *ret = calloc(sizeof(tPTY) + namelen + 1, 1);
	if(!ret) {
		errno = ENOMEM;
		return NULL;
	}
	
	// - List maintainance
	ret->Next = *prev_np;
	*prev_np = ret;
	// - PTY Name (Used by VT)
	ret->Name = (char*)(ret + 1);
	if( idx == -1 )
		strcpy(ret->Name, Name);
	else if( Name[0] )
		sprintf(ret->Name, "%.*s%u", strlen(Name)-1, Name, idx);
	else
		sprintf(ret->Name, "%u", idx);
	ret->NumericName = idx;
	// - Output function and handle (same again)
	ret->OutputHandle = Handle;
	ret->OutputFcn = Output;
	ret->ReqResize = ReqResize;
	ret->ModeSet = ModeSet;
	// - Initialise modes
	if( InitialDims )
		ret->Dims = *InitialDims;
	if( InitialMode )
		ret->Mode = *InitialMode;
	// - Client node
	ret->ClientNode.ImplPtr = ret;
	ret->ClientNode.Type = &gPTY_NodeType_Client;
	ret->ClientNode.UID = Threads_GetUID();
	ret->ClientNode.GID = Threads_GetGID();
	ret->ClientNode.NumACLs = 1;
	ret->ClientNode.ACLs = &ret->OwnerRW;
	// - Owner Read-Write ACL
	ret->OwnerRW.Ent.ID = Threads_GetUID();
	ret->OwnerRW.Perm.Perms = -1;

	if( Name[0] ) {
		giPTY_NamedCount ++;
		RWLock_Release(&glPTY_NamedPTYs);
	}
	else {
		giPTY_NumCount ++;
		RWLock_Release(&glPTY_NumPTYs);	
	}

	return ret;
}

int PTY_SetAttrib(tPTY *PTY, const struct ptydims *Dims, const struct ptymode *Mode, int WasClient)
{
	if( Mode )
	{
		// (for now) userland terminals can't be put into framebuffer mode
		// - Userland PTYs are streams, framebuffer is a block
		if( !PTY->OutputFcn && (Mode->OutputMode & PTYOMODE_BUFFMT) == PTYBUFFMT_FB ) {
			errno = EINVAL;
			return -1;
		}
		if( WasClient )
		{
			if( PTY->ModeSet && PTY->ModeSet(PTY->OutputHandle, Mode) )
			{
				errno = EINVAL;
				return -1;
			}
			else if( !PTY->OutputFcn )
			{
				Log_Warning("PTY", "TODO: Inform server of client SETMODE, halt output");
				// Block slave write until master ACKs
				// 0-length read on master indicates need to GETMODE
			}
		}
		else
		{
			// Should the client be informed that the server just twiddled the modes?
			Log_Warning("PTY", "Server changed mode, TODO: inform client?");
		}
		LOG("PTY %p mode set to {0%o, 0%o}", PTY, Mode->InputMode, Mode->OutputMode);
		PTY->Mode = *Mode;
	}
	if( Dims )
	{
		if( WasClient )
		{
			// Poke the server?
			if( PTY->ReqResize && PTY->ReqResize(PTY->OutputHandle, Dims) )
			{
				errno = EINVAL;
				return -1;
			}
			else if( !PTY->OutputFcn )
			{
				// Inform server process... somehow
				Log_Warning("PTY", "TODO: Inform server of client resize request");
			}
		}
		else
		{
			// SIGWINSZ to client
			if( PTY->ControllingProcGroup > 0 )
				Threads_SignalGroup(PTY->ControllingProcGroup, SIGWINCH);
		}
		LOG("PTY %p dims set to %ix%i", PTY, Dims->W, Dims->H);
		PTY->Dims = *Dims;
	}
	return 0;
}

void PTY_Close(tPTY *PTY)
{
	
}

size_t _rb_write(void *buf, size_t buflen, int *rd, int *wr, const void *data, size_t len)
{
	size_t space = (*rd - *wr + buflen - 1) % buflen;
	len = MIN(space, len);
	if(*wr + len >= buflen) {
		size_t prelen = buflen - *wr;
		memcpy((char*)buf + *wr, data, prelen);
		memcpy(buf, (char*)data + prelen, len - prelen);
		*wr = len - prelen;
	}
	else {
		memcpy((char*)buf + *wr, data, len);
		*wr += len;
	}
	return len;
}
size_t _rb_read(void *buf, size_t buflen, int *rd, int *wr, void *data, size_t len)
{
	size_t space = (*wr - *rd + buflen) % buflen;
	len = MIN(space, len);
	if(*rd + len >= buflen) {
		size_t prelen = buflen - *rd;
		memcpy(data, (char*)buf + *rd, prelen);
		memcpy((char*)data + prelen, buf, len - prelen);
		*rd = len - prelen;
	}
	else {
		memcpy(data, (char*)buf + *rd, len);
		*rd += len;
	}
	return len;
}

size_t PTY_int_WriteInput(tPTY *PTY, const char *Input, size_t Length)
{
	size_t	ret;

	Mutex_Acquire(&PTY->InputMutex);	

	ret = _rb_write(PTY->InputData, INPUT_RINGBUFFER_LEN, &PTY->InputReadPos, &PTY->InputWritePos,
		Input, Length);
	
	Mutex_Release(&PTY->InputMutex);

	VFS_MarkAvaliable(&PTY->ClientNode, 1);
	if(ret < Length && PTY->ServerNode)
		VFS_MarkFull(PTY->ServerNode, 1);	

	return ret;
}

size_t PTY_int_SendInput(tPTY *PTY, const char *Input, size_t Length)
{
	size_t	ret = 1, print = 1;
	
	// Input mode stuff only counts for text output mode
	// - Any other mode sends Uint32 keypresses
	if( (PTY->Mode.OutputMode & PTYOMODE_BUFFMT) != PTYBUFFMT_TEXT )
		return PTY_int_WriteInput(PTY, Input, Length);
	// If in raw mode, flush directly
	if( (PTY->Mode.InputMode & PTYIMODE_RAW) )
		return PTY_int_WriteInput(PTY, Input, Length);
	
	if( PTY->Mode.InputMode & PTYIMODE_CANON )
	{
		
		switch(Input[0])
		{
		case 3:	// INTR - ^C
			// Send SIGINT
			if( PTY->ControllingProcGroup > 0 )
				Threads_SignalGroup(PTY->ControllingProcGroup, SIGINT);
			print = 0;
			break;
		case 4:	// EOF - ^D
			PTY_int_WriteInput(PTY, PTY->LineData, PTY->LineLength);
			PTY->HasHitEOF = (PTY->LineLength == 0);
			PTY->LineLength = 0;
			print = 0;
			break;
		case 8:	// Backspace
			if(PTY->LineLength != 0) {
				PTY->LineLength --;
				PTY_WriteClient(&PTY->ClientNode, 0, 3, "\b \b", 0);
			}
			print = 0;
			break;
		case 'w'-'a':	// Word erase
			while(PTY->LineLength != 0 && isalnum(PTY->LineData[--PTY->LineLength]))
				PTY_WriteClient(&PTY->ClientNode, 0, 1, "\b", 0);
			PTY_WriteClient(&PTY->ClientNode, 0, 3, "\x1b[K", 0);
			print = 0;
			break;
		case 'u'-'a':	// Kill
			PTY_WriteClient(&PTY->ClientNode, 0, 8, "\x1b[2K\x1b[0G", 0);
			PTY->LineLength = 0;
			print = 0;
			break;
		case 'v'-'a':
			Input ++;
			Length --;
			ret ++;
			goto _default;
		case '\0':
		case '\n':
			if(PTY->LineLength == INPUT_LINE_LEN) {
				PTY_int_WriteInput(PTY, PTY->LineData, PTY->LineLength);
				PTY->LineLength = 0;
			}
			PTY->LineData[PTY->LineLength++] = '\n';
			PTY_int_WriteInput(PTY, PTY->LineData, PTY->LineLength);
			PTY->LineLength = 0;
			break;
		// TODO: Handle ^[[D and ^[[C for in-line editing, also ^[[1~/^[[4~ (home/end)
		//case 0x1B:
		//	break;
		default:
		_default:
			if(PTY->LineLength == INPUT_LINE_LEN) {
				PTY_int_WriteInput(PTY, PTY->LineData, PTY->LineLength);
				PTY->LineLength = 0;
			}
			PTY->LineData[PTY->LineLength++] = Input[0];
			break;
		}
	}
	else
	{
		#if 0
		if( PTY->Mode.InputMode & PTYIMODE_NLCR )
		{
			if( Input[0] == '\n' ) {
				char ch = '\r';
				ret = PTY_int_WriteInput(PTY, &ch, 1);
			}
			else {
				 int	i;
				for( i = 0; i < Length && Input[i] != '\n'; i ++ )
					;
				ret = PTY_int_WriteInput(PTY, Input, i);
			}
		}
		// TODO: CRNL mode?
		else
		#endif
			ret = PTY_int_WriteInput(PTY, Input, Length);
	}
	
	// Echo if requested
	if( PTY->Mode.InputMode & PTYIMODE_ECHO )
	{
		PTY_WriteClient(&PTY->ClientNode, 0, print, Input, 0);
	}
	
	return ret;
}

size_t PTY_SendInput(tPTY *PTY, const char *Input, size_t Length)
{
	size_t ret = 0;
	while( ret < Length && !PTY->ClientNode.BufferFull )
	{
		// TODO: Detect blocking?
		ret += PTY_int_SendInput(PTY, Input + ret, Length - ret);
	}
	return ret;
}

// --- VFS ---
int PTY_ReadDir(tVFS_Node *Node, int Pos, char Name[FILENAME_MAX])
{
	tPTY	*pty = NULL;
	 int	idx = Pos;
	if( idx < giPTY_NumCount )
	{
		RWLock_AcquireRead(&glPTY_NumPTYs);
		for( pty = gpPTY_FirstNumPTY; pty && idx; pty = pty->Next )
			idx --;
		RWLock_Release(&glPTY_NumPTYs);
	}
	else if( idx < (giPTY_NumCount + giPTY_NamedCount) )
	{
		idx -= giPTY_NumCount;
		RWLock_AcquireRead(&glPTY_NamedPTYs);
		for( pty = gpPTY_FirstNamedPTY; pty && idx; pty = pty->Next )
			idx --;
		RWLock_Release(&glPTY_NamedPTYs);
	}

	if( !pty ) {
		return -1;
	}

	strncpy(Name, pty->Name, FILENAME_MAX);
	return 0;
}

tVFS_Node *PTY_FindDir(tVFS_Node *Node, const char *Name, Uint Flags)
{
	char	*end;
	 int	num = strtol(Name, &end, 10);

	if( strcmp(Name, "ptmx") == 0 ) {
		tVFS_Node	*ret = calloc(sizeof(tVFS_Node), 1);
		ret->Size = -1;
		ret->Type = &gPTY_NodeType_Server;
		return ret;
	} 
	
	if( Name[0] == '\0' )
		return NULL;
	
	tPTY	*ret = NULL;
	if( num && end[0] == '\0' )
	{
		// Numeric name
		RWLock_AcquireRead(&glPTY_NumPTYs);
		for( tPTY *pty = gpPTY_FirstNumPTY; pty; pty = pty->Next )
		{
			if( pty->NumericName > num )
				break;
			if( pty->NumericName == num ) {
				ret = pty;
				break;
			}
		}
		RWLock_Release(&glPTY_NumPTYs);
	}
	else
	{
		// String name
		RWLock_AcquireRead(&glPTY_NamedPTYs);
		for( tPTY *pty = gpPTY_FirstNamedPTY; pty; pty = pty->Next )
		{
			int cmp = strcmp(pty->Name, Name);
			if(cmp > 0)
				break;
			if(cmp == 0 ) {
				ret = pty;
				break;
			}
		}
		RWLock_Release(&glPTY_NamedPTYs);
	}
//	Debug("PTY_FindDir('%s') returned %p", Name, &ret->ClientNode);
	if( ret ) {
		tVFS_Node	*retnode = &ret->ClientNode;
		retnode->ReferenceCount ++;
		return retnode;
	}
	else
		return NULL;
}

//\! Read from the client's input
size_t PTY_ReadClient(tVFS_Node *Node, off_t Offset, size_t Length, void *Buffer, Uint Flags)
{
	tPTY *pty = Node->ImplPtr;
	
	// Read from flushed queue
	tTime	timeout_z = 0, *timeout = (Flags & VFS_IOFLAG_NOBLOCK) ? &timeout_z : NULL;
	 int	rv;
_select:
	// If server has disconnected, return EIO
	if( pty->ServerNode && pty->ServerNode->ReferenceCount == 0 ) {
		//Threads_PostSignal(SIGPIPE);
		errno = EIO;
		return -1;
	}
	// Wait for data to be ready
	rv = VFS_SelectNode(Node, VFS_SELECT_READ, timeout, "PTY_ReadClient");
	if(!rv) {
		errno = (timeout ? EWOULDBLOCK : EINTR);
		return -1;
	}

	Mutex_Acquire(&pty->InputMutex);
	Length = _rb_read(pty->InputData, INPUT_RINGBUFFER_LEN, &pty->InputReadPos, &pty->InputWritePos,
		Buffer, Length);
	if( Length && pty->ServerNode )
		VFS_MarkFull(pty->ServerNode, 0);
	Mutex_Release(&pty->InputMutex);

	if(pty->InputReadPos == pty->InputWritePos)
		VFS_MarkAvaliable(Node, 0);

	if(Length == 0 && !pty->HasHitEOF) {
		goto _select;
	}
	pty->HasHitEOF = 0;

	return Length;
}

//\! Write to the client's output
size_t PTY_WriteClient(tVFS_Node *Node, off_t Offset, size_t Length, const void *Buffer, Uint Flags)
{
	tPTY *pty = Node->ImplPtr;

	// If the server has terminated, send SIGPIPE
	if( pty->ServerNode && pty->ServerNode->ReferenceCount == 0 )
	{
		Threads_PostSignal(SIGPIPE);
		errno = EIO;
		return -1;
	}	

	// Write to either FIFO or directly to output function
	if( pty->OutputFcn ) {
		pty->OutputFcn(pty->OutputHandle, Length, Buffer);
		return Length;
	}
	
	// FIFO
	size_t remaining = Length;
	do
	{
		tTime	timeout_z, *timeout = (Flags & VFS_IOFLAG_NOBLOCK) ? &timeout_z : NULL;
		 int	rv;
		
		rv = VFS_SelectNode(Node, VFS_SELECT_WRITE, timeout, "PTY_WriteClient");
		if(!rv) {
			errno = (timeout ? EWOULDBLOCK : EINTR);
			return -1;
		}
		
		// Write to output ringbuffer
		size_t written = _rb_write(pty->OutputData, OUTPUT_RINGBUFFER_LEN,
			&pty->OutputReadPos, &pty->OutputWritePos,
			Buffer, remaining);
		LOG("Wrote %i of %i : '%.*s'", written, remaining, written, Buffer);
		VFS_MarkAvaliable(pty->ServerNode, 1);
		if( (pty->OutputWritePos + 1) % OUTPUT_RINGBUFFER_LEN == pty->OutputReadPos )
			VFS_MarkFull(Node, 1);
		
		remaining -= written;
		Buffer = (const char*)Buffer + written;
	} while( remaining > 0 || (Flags & VFS_IOFLAG_NOBLOCK) );
	
	return Length - remaining;
}

void PTY_ReferenceClient(tVFS_Node *Node)
{
	Node->ReferenceCount ++;
}

void PTY_CloseClient(tVFS_Node *Node)
{
	tPTY	*pty = Node->ImplPtr;
	Node->ReferenceCount --;

	// Remove PID from list
	// TODO: Maintain list of client processes

	// Free structure if this was the last open handle
	if( Node->ReferenceCount > 0 )
		return ;
	if( pty->ServerNode && pty->ServerNode->ReferenceCount == 0 )
	{
		// Free the structure! (Should be off the PTY list now)
		free(pty->ServerNode);
		free(pty);
	}
}

//\! Read from the client's output
size_t PTY_ReadServer(tVFS_Node *Node, off_t Offset, size_t Length, void *Buffer, Uint Flags)
{
	tPTY *pty = Node->ImplPtr;
	if( !pty ) {
		errno = EIO;
		return -1;
	}

	// TODO: Prevent two servers fighting over client's output	
	if( pty->OutputFcn )
	{
		// Kernel-land PTYs can't be read from userland
		return 0;
	}
	
	// Read back from fifo
	tTime	timeout_z = 0, *timeout = (Flags & VFS_IOFLAG_NOBLOCK) ? &timeout_z : NULL;
	int rv = VFS_SelectNode(Node, VFS_SELECT_READ, timeout, "PTY_ReadServer");
	if(!rv) {
		errno = (timeout ? EWOULDBLOCK : EINTR);
		return -1;
	}
	
	Length = _rb_read(pty->OutputData, OUTPUT_RINGBUFFER_LEN,
		&pty->OutputReadPos, &pty->OutputWritePos,
		Buffer, Length);
	LOG("Read %i '%.*s'", Length, Length, Buffer);
	if( pty->OutputReadPos == pty->OutputWritePos )
		VFS_MarkAvaliable(Node, 0);
	VFS_MarkFull(&pty->ClientNode, 0);
	
	return Length;
}

//\! Write to the client's input
size_t PTY_WriteServer(tVFS_Node *Node, off_t Offset, size_t Length, const void *Buffer, Uint Flags)
{
	tPTY	*pty = Node->ImplPtr;
	if( !pty ) {
		errno = EIO;
		return -1;
	}

	tTime	timeout_z = 0, *timeout = (Flags & VFS_IOFLAG_NOBLOCK) ? &timeout_z : NULL;
	int rv = VFS_SelectNode(Node, VFS_SELECT_WRITE, timeout, "PTY_WriteServer");
	if(!rv) {
		errno = (timeout ? EWOULDBLOCK : EINTR);
		return -1;
	}
	size_t	used = 0;
	do {
		used += PTY_SendInput(Node->ImplPtr, Buffer, Length);
	} while( used < Length && !(Flags & VFS_IOFLAG_NOBLOCK) );
	
	if( (pty->InputWritePos+1)%INPUT_RINGBUFFER_LEN == pty->InputReadPos )
		VFS_MarkFull(Node, 1);
	return used;
}

void PTY_CloseServer(tVFS_Node *Node)
{
	tPTY	*pty = Node->ImplPtr;
	// Dereference node
	Node->ReferenceCount --;
	// If reference count == 0, remove from main list
	if( Node->ReferenceCount > 0 )
		return ;
	
	// Locate on list and remove
	tPTY	**prev_np;
	if( pty->NumericName == -1 ) {
		RWLock_AcquireWrite(&glPTY_NamedPTYs);
		prev_np = &gpPTY_FirstNamedPTY;
	}
	else {
		RWLock_AcquireWrite(&glPTY_NumPTYs);
		prev_np = &gpPTY_FirstNumPTY;
	}

	// Search list until *prev_np is equal to pty	
	for( tPTY *tmp = *prev_np; *prev_np != pty && tmp; prev_np = &tmp->Next, tmp = tmp->Next )
		;
	
	// Remove
	if( *prev_np != pty ) {
		Log_Error("PTY", "PTY %p(%i/%s) not on list at deletion time", pty, pty->NumericName, pty->Name);
	}
	else {
		*prev_np = pty->Next;
	}
	
	// Clean up lock
	if( pty->NumericName == -1 ) {
		RWLock_Release(&glPTY_NamedPTYs);
		giPTY_NamedCount --;
	}
	else {
		RWLock_Release(&glPTY_NumPTYs);
		giPTY_NumCount --;
	}

	// Send SIGHUP to controling PGID
	if( pty->ControllingProcGroup > 0 ) {
		Threads_SignalGroup(pty->ControllingProcGroup, SIGHUP);
	}

	// If there are no open children, we can safely free this PTY
	if( pty->ClientNode.ReferenceCount == 0 ) {
		free(Node);
		free(pty);
	}
}

int PTY_IOCtl(tVFS_Node *Node, int ID, void *Data)
{
	tPTY	*pty = Node->ImplPtr;
	struct ptymode	*mode = Data;
	struct ptydims	*dims = Data;
	
	int	is_server = !pty || Node == pty->ServerNode;

	LOG("(%i,%p) %s", ID, Data, (is_server?"Server":"Client"));

	switch(ID)
	{
	case DRV_IOCTL_TYPE:	return DRV_TYPE_TERMINAL;
	case DRV_IOCTL_IDENT:	memcpy(Data, "PTY\0", 4);	return 0;
	case DRV_IOCTL_VERSION:	return 0x100;
	case DRV_IOCTL_LOOKUP:	return 0;
	
	case PTY_IOCTL_GETMODE:
		if( !pty )	return 0;
		if( !CheckMem(Data, sizeof(*mode)) ) { errno = EINVAL; return -1; }
		*mode = pty->Mode;
		// TODO: ACK client's SETMODE
		return 0;
	case PTY_IOCTL_SETMODE:
		if( !pty )	return 0;
		if( !CheckMem(Data, sizeof(*mode)) ) { errno = EINVAL; return -1; }
		PTY_SetAttrib(pty, NULL, mode, !is_server);
		return 0;
	case PTY_IOCTL_GETDIMS:
		if( !pty )	return 0;
		if( !CheckMem(Data, sizeof(*dims)) ) { errno = EINVAL; return -1; }
		*dims = pty->Dims;
		return 0;
	case PTY_IOCTL_SETDIMS:
		if( !pty )	return 0;
		if( !CheckMem(Data, sizeof(*dims)) ) { errno = EINVAL; return -1; }
		PTY_SetAttrib(pty, dims, NULL, !is_server);
		return 0;
	case PTY_IOCTL_GETID:
		if( pty )
		{
			size_t	len = strlen(pty->Name)+1;
			if( Data )
			{
				if( !CheckMem(Data, len) ) { errno = EINVAL; return -1; }
				strcpy(Data, pty->Name);
			}
			return len;
		}
		return 0;
	case PTY_IOCTL_SETID:
		if( Data && !CheckString(Data) ) { errno = EINVAL; return -1; }
		if( pty )	return EALREADY;
		pty = PTY_Create(Data, NULL, NULL,NULL, NULL, NULL,NULL);
		if(pty == NULL)
			return 1;
		Node->ImplPtr = pty;
		pty->ServerNode = Node;
		return 0;
	case PTY_IOCTL_SETPGRP:
		// TODO: Should this only be done by client?
		if( Data )
		{
			if( !CheckMem(Data, sizeof(tPGID)) ) { errno = EINVAL; return -1; }
			pty->ControllingProcGroup = *(tPGID*)Data;
			Log_Debug("PTY", "Set controlling PGID to %i", pty->ControllingProcGroup);
		}
		return pty->ControllingProcGroup;
	}
	errno = ENOSYS;
	return -1;
}


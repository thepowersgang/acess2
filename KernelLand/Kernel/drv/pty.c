/*
 * Acess2 Kernel
 * - By John Hodge (thePowersGang)
 *
 * drv/pty.c
 * - Pseudo Terminals
 */
#define DEBUG	1
#include <acess.h>
#include <vfs.h>
#include <fs_devfs.h>
#include <drv_pty.h>
#include <modules.h>
#include <rwlock.h>
#include <mutex.h>

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
	
	tVFS_Node	ClientNode;
	tVFS_Node	ServerNode;
	tVFS_ACL	OwnerRW;
	
	// TODO: Maintain list of client PIDs
};

// === PROTOTYPES ===
 int	PTY_Install(char **Arguments);
 int	PTY_ReadDir(tVFS_Node *Node, int Pos, char Name[FILENAME_MAX]);
tVFS_Node	*PTY_FindDir(tVFS_Node *Node, const char *Name, Uint Flags);
tVFS_Node	*PTY_MkNod(tVFS_Node *Node, const char *Name, Uint Mode);

size_t	_rb_write(void *buf, size_t buflen, int *rd, int *wr, const void *data, size_t len);
size_t	_rb_read(void *buf, size_t buflen, int *rd, int *wr, void *data, size_t len);
size_t	PTY_int_WriteInput(tPTY *PTY, const char *Input, size_t Length);
size_t	PTY_int_SendInput(tPTY *PTY, const char *Input, size_t Length);
// PTY_SendInput
size_t	PTY_ReadClient(tVFS_Node *Node, off_t Offset, size_t Length, void *Buffer, Uint Flags);
size_t	PTY_WriteClient(tVFS_Node *Node, off_t Offset, size_t Length, const void *Buffer, Uint Flags);
 int	PTY_IOCtlClient(tVFS_Node *Node, int ID, void *Arg);
void	PTY_ReferenceClient(tVFS_Node *Node);
void	PTY_CloseClient(tVFS_Node *Node);
size_t	PTY_ReadServer(tVFS_Node *Node, off_t Offset, size_t Length, void *Buffer, Uint Flags);
size_t	PTY_WriteServer(tVFS_Node *Node, off_t Offset, size_t Length, const void *Buffer, Uint Flags);
 int	PTY_IOCtlServer(tVFS_Node *Node, int ID, void *Arg);
void	PTY_CloseServer(tVFS_Node *Node);

// === GLOBALS ===
MODULE_DEFINE(0, 0x100, PTY, PTY_Install, NULL, NULL);
tVFS_NodeType	gPTY_NodeType_Root = {
	.TypeName = "PTY-Root",
	.ReadDir = PTY_ReadDir,
	.FindDir = PTY_FindDir,
	.MkNod = PTY_MkNod
};
tVFS_NodeType	gPTY_NodeType_Client = {
	.TypeName = "PTY-Client",
	.Read = PTY_ReadClient,
	.Write = PTY_WriteClient,
	.IOCtl = PTY_IOCtlClient,
	.Reference = PTY_ReferenceClient,
	.Close = PTY_CloseClient
};
tVFS_NodeType	gPTY_NodeType_Server = {
	.TypeName = "PTY-Server",
	.Read = PTY_ReadServer,
	.Write = PTY_WriteServer,
	.IOCtl = PTY_IOCtlServer,
	.Close = PTY_CloseServer
};
tDevFS_Driver	gPTY_Driver = {
	.Name = "pts",
	.RootNode = {
		.Flags = VFS_FFLAG_DIRECTORY,
		.Type = &gPTY_NodeType_Root
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
tPTY *PTY_Create(const char *Name, void *Handle, tPTY_OutputFcn Output, tPTY_ReqResize ReqResize)
{
	tPTY	**prev_np = NULL;
	size_t	namelen;
	 int	idx = 1;
	if( Name && Name[0] )
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
	else
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
		namelen = 0;
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
	if(Name)
		strcpy(ret->Name, Name);
	else
		ret->Name[0] = 0;
	ret->NumericName = idx;
	// - Output function and handle (same again)
	ret->OutputHandle = Handle;
	ret->OutputFcn = Output;
	ret->ReqResize = ReqResize;
	// - Server node
	ret->ServerNode.ImplPtr = ret;
	ret->ServerNode.Type = &gPTY_NodeType_Server;
	ret->ServerNode.UID = Threads_GetUID();
	ret->ServerNode.GID = Threads_GetGID();
	ret->ServerNode.NumACLs = 1;
	ret->ServerNode.ACLs = &ret->OwnerRW;
	ret->ServerNode.ReferenceCount = (Output ? 1 : 0);	// Prevent a userland close killing a kernel pty
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

	if( Name && Name[0] ) {
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
		if( !PTY->OutputFcn && (Mode->OutputMode & PTYOMODE_BUFFMT) == PTYBUFFMT_FB ) {
			errno = EINVAL;
			return -1;
		}
		PTY->Mode = *Mode;
		if( !WasClient && !PTY->OutputFcn )
		{
			Log_Warning("PTY", "TODO: Need to stop client output until modeset has been ACKed");
			// Block write until acked
			// ACK by server doing GETMODE
		}
	}
	if( Dims )
	{
		if( WasClient ) {
			// Poke the server?
			if( PTY->ReqResize && PTY->ReqResize(PTY->OutputHandle, Dims) )
			{
				errno = EINVAL;
				return -1;
			}
			else if( !PTY->OutputFcn )
			{
				// Inform server process... somehow
			}
		}
		else {
			// SIGWINSZ to client
		}
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
	ENTER("pbuf ibuflen prd pwr pdata ilen", buf, buflen, rd, wr, data, len);
	len = MIN(space, len);
	LOG("space = %i, *rd = %i, *wr = %i", space, *rd, *wr);
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
	LEAVE('i', len);
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
	if(ret < Length)
		VFS_MarkFull(&PTY->ServerNode, 1);	

	return ret;
}

size_t PTY_int_SendInput(tPTY *PTY, const char *Input, size_t Length)
{
	size_t	ret = 1, print = 1;
	
	// Input mode stuff only counts for text output mode
	// - Any other is Uint32 keypresses
	if( (PTY->Mode.OutputMode & PTYOMODE_BUFFMT) != PTYBUFFMT_TEXT )
		return PTY_int_WriteInput(PTY, Input, Length);
	// If in raw mode, flush directlr
	if( (PTY->Mode.InputMode & PTYIMODE_RAW) )
		return PTY_int_WriteInput(PTY, Input, Length);
	
	if( PTY->Mode.InputMode & PTYIMODE_CANON )
	{
		const char	char_bs = '\b';
		switch(Input[0])
		{
		case 3:	// INTR - ^C
			// TODO: Send SIGINT
			// Threads_PostSignalExt(PTY->ClientThreads, SIGINT);
			print = 0;
			break;
		case 4:	// EOF - ^D
			PTY_int_WriteInput(PTY, PTY->LineData, PTY->LineLength);
			PTY->HasHitEOF = (PTY->LineLength == 0);
			PTY->LineLength = 0;
			print = 0;
			break;
		case 8:	// Backspace
			if(PTY->LineLength != 0)
				PTY->LineLength --;
			break;
		case 'w'-'a':	// Word erase
			while(PTY->LineLength != 0 && isalnum(PTY->LineData[--PTY->LineLength]))
				PTY_WriteClient(&PTY->ClientNode, 0, 1, &char_bs, 0);
			print = 0;
			break;
		case 'u'-'a':	// Kill
			while(PTY->LineLength > 0)
				PTY_WriteClient(&PTY->ClientNode, 0, 1, &char_bs, 0);
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
	if( Pos < giPTY_NumCount * 2 )
	{
		RWLock_AcquireRead(&glPTY_NumPTYs);
		for( pty = gpPTY_FirstNumPTY; pty; pty = pty->Next )
		{
			if( Pos < 2 )
				break;
			Pos -= 2;
		}
		RWLock_Release(&glPTY_NumPTYs);
	}
	else if( Pos < (giPTY_NumCount + giPTY_NamedCount) * 2 )
	{
		RWLock_AcquireRead(&glPTY_NamedPTYs);
		for( pty = gpPTY_FirstNamedPTY; pty; pty = pty->Next )
		{
			if( Pos < 2 )
				break;
			Pos -= 2;
		}
		RWLock_Release(&glPTY_NamedPTYs);
	}
	

	if( !pty )
		return -1;
	
	if( pty->Name[0] )
		snprintf(Name, FILENAME_MAX, "%s%c", pty->Name, (Pos == 0 ? 'c' : 's'));
	else
		snprintf(Name, FILENAME_MAX, "%i%c", pty->NumericName, (Pos == 0 ? 'c' : 's'));
	return 0;
}

tVFS_Node *PTY_FindDir(tVFS_Node *Node, const char *Name, Uint Flags)
{
	char	*end;
	 int	num = strtol(Name, &end, 10);
	
	if( Name[0] == '\0' || Name[1] == '\0' )
		return NULL;
	
	size_t	len = strlen(Name);
	if( Name[len-1] != 'c' && Name[len-1] != 's' )
		return NULL;
	 int	isServer = (Name[len-1] != 'c');
	
	tPTY	*ret = NULL;
	if( num && (end[0] == 'c' || end[0] == 's') && end[1] == '\0' )
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
			int cmp = strncmp(pty->Name, Name, len-1);
			if(cmp > 0)
				break;
			if(cmp == 0 && pty->Name[len-1] == '\0' ) {
				ret = pty;
				break;
			}
		}
		RWLock_Release(&glPTY_NamedPTYs);
	}
	if( ret )
		return (isServer ? &ret->ServerNode : &ret->ClientNode);
	else
		return NULL;
}

tVFS_Node *PTY_MkNod(tVFS_Node *Node, const char *Name, Uint Mode)
{
	// zero-length name means a numbered pty has been requested
	if( Name[0] == '\0' )
	{
		tPTY	*ret = PTY_Create(NULL, NULL, NULL, NULL);
		if( !ret )
			return NULL;
		return &ret->ServerNode;
	}
	
	// Otherwise return a named PTY
	// TODO: Should the request be for '<name>s' or just '<name>'	

	tPTY	*ret = PTY_Create(Name, NULL, NULL, NULL);
	if(!ret)
		return NULL;
	return &ret->ServerNode;
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
	if( pty->ServerNode.ReferenceCount == 0 ) {
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
	if( pty->ServerNode.ReferenceCount == 0 )
	{
		//Threads_PostSignal(SIGPIPE);
		errno = EIO;
		return -1;
	}	

	// Write to either FIFO or directly to output function
	if( pty->OutputFcn )
	{
		pty->OutputFcn(pty->OutputHandle, Buffer, Length, pty->Mode.OutputMode);
	}
	else
	{
		// Write to output ringbuffer
		Length = _rb_write(pty->OutputData, OUTPUT_RINGBUFFER_LEN,
			&pty->OutputReadPos, &pty->OutputWritePos,
			Buffer, Length);
		VFS_MarkAvaliable(&pty->ServerNode, 1);
	}
	
	return Length;
}

int PTY_IOCtlClient(tVFS_Node *Node, int ID, void *Data)
{
	tPTY	*pty = Node->ImplPtr;
	struct ptymode	*mode = Data;
	struct ptydims	*dims = Data;
	switch(ID)
	{
	case DRV_IOCTL_TYPE:	return DRV_TYPE_TERMINAL;
	case DRV_IOCTL_IDENT:	memcpy(Data, "PTY\0", 4);	return 0;
	case DRV_IOCTL_VER:	return 0x100;
	case DRV_IOCTL_LOOKUP:	return 0;
	
	case PTY_IOCTL_GETMODE:
		if( !CheckMem(Data, sizeof(*mode)) ) { errno = EINVAL; return -1; }
		*mode = pty->Mode;
		return 0;
	case PTY_IOCTL_SETMODE:
		if( !CheckMem(Data, sizeof(*mode)) ) { errno = EINVAL; return -1; }
		return PTY_SetAttrib(pty, NULL, mode, 1);
	case PTY_IOCTL_GETDIMS:
		if( !CheckMem(Data, sizeof(*dims)) ) { errno = EINVAL; return -1; }
		*dims = pty->Dims;
		return 0;
	case PTY_IOCTL_SETDIMS:
		if( !CheckMem(Data, sizeof(*dims)) ) { errno = EINVAL; return -1; }
		return PTY_SetAttrib(pty, dims, NULL, 1);
	}
	errno = ENOSYS;
	return -1;
}

void PTY_ReferenceClient(tVFS_Node *Node)
{
	Node->ReferenceCount ++;
	// TODO: Add PID to list of client PIDs
	Log_Notice("PTY", "TODO: List of client PIDs");
}

void PTY_CloseClient(tVFS_Node *Node)
{
	tPTY	*pty = Node->ImplPtr;
	Node->ReferenceCount --;

	// Remove PID from list

	// Free structure if this was the last open handle
	if( Node->ReferenceCount == 0 && pty->ServerNode.ReferenceCount == 0 )
	{
		// Free the structure! (Should be off the PTY list now)
		free(pty);
	}
}

//\! Read from the client's output
size_t PTY_ReadServer(tVFS_Node *Node, off_t Offset, size_t Length, void *Buffer, Uint Flags)
{
	tPTY *pty = Node->ImplPtr;

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
	if( pty->OutputReadPos == pty->OutputWritePos )
		VFS_MarkAvaliable(Node, 0);
	VFS_MarkFull(&pty->ClientNode, 0);
	
	return Length;
}

//\! Write to the client's input
size_t PTY_WriteServer(tVFS_Node *Node, off_t Offset, size_t Length, const void *Buffer, Uint Flags)
{
	return PTY_SendInput(Node->ImplPtr, Buffer, Length);
}

int PTY_IOCtlServer(tVFS_Node *Node, int ID, void *Data)
{
	tPTY	*pty = Node->ImplPtr;
	struct ptymode	*mode = Data;
	struct ptydims	*dims = Data;
	switch(ID)
	{
	case DRV_IOCTL_TYPE:	return DRV_TYPE_TERMINAL;
	case DRV_IOCTL_IDENT:	memcpy(Data, "PTY\0", 4);	return 0;
	case DRV_IOCTL_VER:	return 0x100;
	case DRV_IOCTL_LOOKUP:	return 0;
	
	case PTY_IOCTL_GETMODE:
		if( !CheckMem(Data, sizeof(*mode)) ) { errno = EINVAL; return -1; }
		*mode = pty->Mode;
		// ACK client's SETMODE
		return 0;
	case PTY_IOCTL_SETMODE:
		if( !CheckMem(Data, sizeof(*mode)) ) { errno = EINVAL; return -1; }
		PTY_SetAttrib(pty, NULL, mode, 0);
		return 0;
	case PTY_IOCTL_GETDIMS:
		if( !CheckMem(Data, sizeof(*dims)) ) { errno = EINVAL; return -1; }
		*dims = pty->Dims;
		return 0;
	case PTY_IOCTL_SETDIMS:
		if( !CheckMem(Data, sizeof(*dims)) ) { errno = EINVAL; return -1; }
		PTY_SetAttrib(pty, dims, NULL, 0);
		break;
	case PTY_IOCTL_GETID:
		return pty->NumericName;
	}
	errno = ENOSYS;
	return -1;
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
	if( pty->Name[0] ) {
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
	if( pty->Name[0] ) {
		RWLock_Release(&glPTY_NamedPTYs);
		giPTY_NamedCount --;
	}
	else {
		RWLock_Release(&glPTY_NumPTYs);
		giPTY_NumCount --;
	}

	// If there are no open children, we can safely free this PTY
	if( pty->ClientNode.ReferenceCount == 0 )
		free(pty);
}


/*
 * Acess2 Native Kernel
 * - Acess kernel emulation on another OS using SDL and UDP
 *
 * net.c
 * - Networking
 */
#define DEBUG	0
#include <acess.h>
#include <vfs.h>
#include <fs_devfs.h>
#include "net_wrap.h"

// === PROTOTYPES ===
 int	Net_Install(char **Arguments);
tVFS_Node	*Net_Root_FindDir(tVFS_Node *Node, const char *Name, Uint Flags);
tVFS_Node	*Net_IFace_FindDir(tVFS_Node *Node, const char *Name, Uint Flags);
tVFS_Node	*Net_Routes_FindDir(tVFS_Node *Node, const char *Name, Uint Flags);

tVFS_Node	*Net_TCPC_Open(int AddrType);
size_t	Net_TCPC_Read(tVFS_Node *Node, off_t Offset, size_t Length, void *Buffer, Uint Flags);
size_t	Net_TCPC_Write(tVFS_Node *Node, off_t Offset, size_t Length, const void *Buffer, Uint Flags);
 int	Net_TCPC_IOCtl(tVFS_Node *Node, int IOCtl, void *Arg);
void	Net_TCPC_Close(tVFS_Node *Node);

static size_t	_getaddrsize(int type);

// === GLOBALS ===
tVFS_NodeType	gNet_NT_Root = {
	.FindDir = Net_Root_FindDir,
};
tVFS_NodeType	gNet_NT_IFace = {
	.FindDir = Net_IFace_FindDir,
};
tVFS_NodeType	gNet_NT_Routes = {
	.FindDir = Net_Routes_FindDir,
};
tVFS_NodeType	gNet_NT_TcpC = {
	.Read = Net_TCPC_Read,
	.Write = Net_TCPC_Write,
	.IOCtl = Net_TCPC_IOCtl,
	.Close = Net_TCPC_Close,
};

tDevFS_Driver	gNet_DevInfo = {
	.Name = "ip",
	.RootNode = {
		.Type = &gNet_NT_Root,
		.Flags = VFS_FFLAG_DIRECTORY,
	}
};
tVFS_Node	gNet_Node_IFace4 = {
	.Type = &gNet_NT_IFace,
	.Flags = VFS_FFLAG_DIRECTORY,
	.ImplInt = 4,
};
tVFS_Node	gNet_Node_Routes = {
	.Type = &gNet_NT_Routes,
	.Flags = VFS_FFLAG_DIRECTORY,
};

// === CODE ===
int Net_Install(char **Arguments)
{
	DevFS_AddDevice(&gNet_DevInfo);
	return 0;
}

tVFS_Node *Net_Root_FindDir(tVFS_Node *Node, const char *Name, Uint Flags)
{
	if( strcmp(Name, "routes") == 0 )
		return &gNet_Node_Routes;
	else if( strcmp(Name, "any4") == 0 )
		return &gNet_Node_IFace4;
	else
		return NULL;
}

tVFS_Node *Net_IFace_FindDir(tVFS_Node *Node, const char *Name, Uint Flags)
{
	if( strcmp(Name, "tcpc") == 0 )
		return Net_TCPC_Open(Node->ImplInt);
	else
		return NULL;
}

tVFS_Node *Net_Routes_FindDir(tVFS_Node *Node, const char *Name, Uint Flags)
{
	if( Name[0] == '@' ) {
		switch( Name[1] )
		{
		case '4':	return &gNet_Node_IFace4;
		default:	return NULL;
		}
	}
	else
		return NULL;
}

typedef struct
{
	 int	AddrType;
	tVFS_Node	Node;
	short	SrcPort;
	short	DstPort;
	void	*DestAddr;
} tNet_TCPC;

tVFS_Node *Net_TCPC_Open(int AddrType)
{
	tNet_TCPC	*ret = calloc(sizeof(tNet_TCPC) + _getaddrsize(AddrType), 1);

	ret->AddrType = AddrType;
	ret->Node.ImplPtr = ret;
	ret->Node.Type = &gNet_NT_TcpC;
	ret->DestAddr = ret + 1;

	return &ret->Node;
}

size_t Net_TCPC_Read(tVFS_Node *Node, off_t Offset, size_t Length, void *Buffer, Uint Flags)
{
	return Net_Wrap_ReadSocket(Node->Data, Length, Buffer);
}

size_t Net_TCPC_Write(tVFS_Node *Node, off_t Offset, size_t Length, const void *Buffer, Uint Flags)
{
	return Net_Wrap_WriteSocket(Node->Data, Length, Buffer);
}

int Net_TCPC_IOCtl(tVFS_Node *Node, int IOCtl, void *Data)
{
	tNet_TCPC	*tcpc = Node->ImplPtr;
	switch(IOCtl)
	{
	case 4:
		if(!Data)
			return tcpc->SrcPort;
		if(Node->Data)	// Connection is open
			return -1;
		// TODO: Checkmem
		tcpc->SrcPort = *(Uint16*)Data;
		return tcpc->SrcPort;
	case 5:
		if(!Data)	return tcpc->DstPort;
		if(Node->Data)	return -1;	// Connection open
		// TODO: Checkmem
		tcpc->DstPort = *(Uint16*)Data;
		return tcpc->DstPort;
	case 6:
		if(Node->Data)	return -1;	// Connection open
		memcpy(tcpc->DestAddr, Data, _getaddrsize(tcpc->AddrType));
		return 0;
	case 7:	// Connect
		Node->Data = Net_Wrap_ConnectTcp(Node, tcpc->SrcPort, tcpc->DstPort,
			tcpc->AddrType, tcpc->DestAddr);
		return (Node->Data == NULL);
	case 8:
		Debug("TODO: TCPC rx buffer length");
		return -1;
	default:
		return -1;
	}
}

void Net_TCPC_Close(tVFS_Node *Node)
{
	tNet_TCPC *tcpc = Node->ImplPtr;
	Node->ReferenceCount --;
	if( Node->ReferenceCount == 0 )
	{
		Net_Wrap_CloseSocket(Node->Data);
		free(tcpc);
	}
}


static size_t _getaddrsize(int type)
{
	switch(type)
	{
	case 4:	return 4;
	case 6:	return 16;
	default:	return 0;
	}
}


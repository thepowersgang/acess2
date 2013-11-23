/*
 * Acess2 Native Kernel
 * - Acess kernel emulation on another OS using SDL and UDP
 *
 * net.c
 * - Networking
 */
#include <unistd.h>
#include <logdebug.h>
#include "net_wrap.h"

typedef struct sNetSocket	tNetSocket;

struct sNetSocket
{
	tNetSocket	*Next;
	 int	FD;
};

tNetSocket	*gNet_Wrap_FirstSocket;
tNetSocket	*gNet_Wrap_LastSocket;

// === CODE ===
void *Net_Wrap_ConnectTcp(void *Node, short SrcPort, short DstPtr, int Type, const void *Addr)
{
	return NULL;
}

size_t Net_Wrap_ReadSocket(void *Handle, size_t Bytes, void *Dest)
{
	tNetSocket	*hdl = Handle;
	if(!hdl)	return -1;
	return read(hdl->FD, Dest, Bytes);
}

size_t Net_Wrap_WriteSocket(void *Handle, size_t Bytes, const void *Dest)
{
	tNetSocket	*hdl = Handle;
	if(!hdl)	return -1;
	return write(hdl->FD, Dest, Bytes);
}

void Net_Wrap_CloseSocket(void *Handle)
{
	// TODO
}


/*
 * Acess2 Networking Stack
 * - By John Hodge (thePowersGang)
 *
 * adapters_int.h
 * - Network Adapter Management (API Header)
 */
#ifndef _IPSTACK__ADAPTERS_INT_H_
#define _IPSTACK__ADAPTERS_INT_H_

#include "adapters.h"
#include "adapters_api.h"

struct sAdapter
{
	struct sAdapter	*Next;

	 int	RefCount;
	 int	Index;
	
	const tIPStack_AdapterType	*Type;
	void	*CardHandle;	

	tVFS_Node	Node;

	char	HWAddr[];
};

extern void	Adapter_SendPacket(tAdapter *Handle, tIPStackBuffer *Buffer);
extern void	Adapter_Watch(tAdapter *Handle);

#endif


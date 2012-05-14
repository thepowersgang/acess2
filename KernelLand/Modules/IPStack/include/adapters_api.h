/*
 * Acess2 Networking Stack
 * - By John Hodge (thePowersGang)
 *
 * adapters_api.h
 * - Network Adapter Management (API Header)
 */
#ifndef _IPSTACK__ADAPTERS_API_H_
#define _IPSTACK__ADAPTERS_API_H_

#include "buffer.h"

typedef struct sIPStack_AdapterType tIPStack_AdapterType;

struct sIPStack_AdapterType
{
	 int	Type;
	Uint	Flags;
	const char	*Name;
	
	 int	(*SendPacket)(void *Card, tIPStackBuffer *Buffer);
	tIPStackBuffer	*(*WaitForPacket)(void *Card);
};

extern void	*IPStack_Adapter_Add(const tIPStack_AdapterType *Type, void *Ptr, const void *HWAddr);
extern void	IPStack_Adapter_Del(void *Handle);

#endif


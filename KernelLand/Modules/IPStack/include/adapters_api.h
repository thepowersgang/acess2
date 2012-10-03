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

enum eIPStack_AdapterTypes
{
	ADAPTERTYPE_ETHERNET_10M,
	ADAPTERTYPE_ETHERNET_100M,
	ADAPTERTYPE_ETHERNET_1G
};

// Checksum offloading
#define ADAPTERFLAG_OFFLOAD_MAC	(1 <<  0)
#define ADAPTERFLAG_OFFLOAD_IP4	(1 <<  1)
#define ADAPTERFLAG_OFFLOAD_IP6	(1 <<  2)
#define ADAPTERFLAG_OFFLOAD_TCP	(1 <<  3)
#define ADAPTERFLAG_OFFLOAD_UDP	(1 <<  4)

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


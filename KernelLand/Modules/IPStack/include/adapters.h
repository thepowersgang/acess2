/*
 * Acess2 Networking Stack
 * - By John Hodge (thePowersGang)
 *
 * adapters.h
 * - Network Adapter Management (IPStack Internal)
 */
#ifndef _IPSTACK__ADAPTERS_H_
#define _IPSTACK__ADAPTERS_H_

#include "adapters_api.h"

extern tAdapter	*Adapter_GetByName(const char *Name);
extern void	Adapter_SendPacket(tAdapter *Handle, tIPStackBuffer *Buffer);


#endif


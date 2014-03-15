/*
 */
#ifndef _HWADDR_CACHE_H_
#define _HWADDR_CACHE_H_

extern tMacAddr	HWCache_Resolve(tInterface *Interface, const void *L2Addr);
extern void	HWCache_Set(tAdapter *Adapter, int AddrType, const void *L2Addr, const tMacAddr *HWAddr);


extern void	ARP_Request4(tInterface *Interface, tIPv4 Address);
extern void	ICMPv6_RequestND(tInterface *Interface, const tIPv6 *Address);

#endif


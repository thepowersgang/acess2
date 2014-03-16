/*
 */
#ifndef _ARP_H_
#define _ARP_H_

#include <stddef.h>
#include <stdbool.h>


extern void ARP_SendRequest(int IfNum, const void *IPv4Addr);
extern void ARP_SendResponse(int IfNum, const void *IPv4Addr, const void *MacAddr);
extern bool ARP_Pkt_IsResponse(size_t Len, const void *Packet, const void *ExpectedIP, const void *ExpectedMac);
extern bool ARP_Pkt_IsRequest(size_t Len, const void *Packet, const void *ExpectedIP);

#endif


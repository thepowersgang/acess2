/*
 */
#include "arp.h"
#include "net.h"
#include <stdint.h>
#include <string.h>
#include "test.h"
#include "tests.h"

// === CODE ===
void ARP_SendRequest(int IfNum, const void *IPv4Addr)
{
	const uint8_t	*addr = IPv4Addr;
	uint8_t	pkt[] = {
		// Ethernet
		0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		HOST_MAC,
		0x08,0x06,
		// ARP
		0x00,0x01, 0x08,0x00,
		6,4, 0,1,
		HOST_MAC,
		HOST_IP,
		0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		addr[0],addr[1],addr[2],addr[3],
	};
	Net_Send(IfNum, sizeof(pkt), pkt);
}

void ARP_SendResponse(int IfNum, const void *IPv4Addr, const void *MacAddr)
{
	
}

bool ARP_Pkt_IsResponse(size_t Len, const void *Packet, const void *ExpectedIP, const void *ExpectedMac)
{
	const uint8_t	*pkt8 = Packet;
	if( Len < 2*6+2 ) {
		TEST_WARN("Undersized");
		return false;
	}
	// Ethernet ARP id
	if( pkt8[12+0] != 0x08 || pkt8[12+1] != 0x06 ) {
		TEST_WARN("Ethernet tag %02x %02x != 08 06", pkt8[12+0], pkt8[12+1]);
		return false;
	}
	
	// ARP HWtype/ProtoType
	if( pkt8[14+0] != 0x00 || pkt8[14+1] != 0x01 || pkt8[14+2] != 0x08 || pkt8[14+3] != 0x00 ) {
		TEST_WARN("ARP Types %02x %02x %02x %02x != 00 01 08 00",
			pkt8[14+0], pkt8[14+1], pkt8[14+2], pkt8[14+3]);
		return false;
	}
	// ARP Sizes (HW/Proto) and operation (Response)
	if( pkt8[14+4] != 6 || pkt8[14+5] != 4 || pkt8[14+6] != 0 || pkt8[14+7] != 2 ) {
		TEST_WARN("Sizes+op %02x %02x %02x %02x != 06 04 00 02",
			pkt8[14+4], pkt8[14+5], pkt8[14+6], pkt8[14+7]);
		return false;
	}
	
	if( memcmp(pkt8+14+8, ExpectedMac, 6) != 0 )
		return false;
	if( memcmp(pkt8+14+14, ExpectedIP, 4) != 0 )
		return false;
	if( memcmp(pkt8+14+18, (char[]){HOST_MAC}, 6) != 0 )
		return false;
	if( memcmp(pkt8+14+24, (char[]){HOST_IP}, 4) != 0 )
		return false;
	
	return true;
}

bool ARP_Pkt_IsRequest(size_t Len, const void *Packet, const void *ExpectedIP)
{
	return false;
}


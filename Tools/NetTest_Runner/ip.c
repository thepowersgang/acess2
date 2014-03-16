/*
 */
#include "common.h"
#include "ip.h"
#include "link.h"
#include <assert.h>
#include <string.h>
#include "test.h"

// === STRUCTURES ===
typedef struct {
	uint8_t 	VerLen;
	uint8_t 	DiffServices;
	uint16_t	TotalLength;
	uint16_t	Identifcation;
	uint16_t 	FragmentInfo;	// rsvd:dont:more:[13]offset/8
	uint8_t 	TTL;
	uint8_t 	Protocol;
	uint16_t	HeaderChecksum;
	uint8_t 	SrcAddr[4];
	uint8_t 	DstAddr[4];
	uint8_t 	Options[];
} __attribute__((packed)) tIPv4Hdr;

// === CODE ===
uint16_t IP_Checksum(uint16_t Prev, size_t Length, const void *Data)
{
	//test_trace_hexdump("IP Checksum", Data, Length);
	
	const uint16_t	*words = Data;
	uint32_t	ret = 0;
	for( int i = 0; i < Length/2; i ++ )
	{
		ret += ntohs(*words);
		words ++;
	}
	if( Length & 1 )
		ret += ntohs(*(uint8_t*)words);
	
	while( ret >> 16 )
		ret = (ret & 0xFFFF) + (ret >> 16);
	
	//test_trace("IP Checksum = %04x + 0x%x", ret, (~Prev) & 0xFFFF);
	
	ret += (~Prev) & 0xFFFF;
	while( ret >> 16 )
		ret = (ret & 0xFFFF) + (ret >> 16);
	
	return ~ret;
}

void IP_Send(int IfNum, int AF, const void *Src, const void *Dst, uint8_t proto,
	int BufCount, size_t BufLens[], const void *Bufs[])
{
	size_t	total_length = 0;

	size_t	out_buflens[BufCount+1];
	const void *out_bufs[BufCount+1];
	
	for( int i = 0; i < BufCount; i ++ )
	{
		total_length += BufLens[i];
		out_buflens[1+i] = BufLens[i];
		out_bufs[1+i] = Bufs[i];
	}

	if( AF == 4 )
	{
		tIPv4Hdr	hdr;
		hdr.VerLen = (4 << 4) | (sizeof(hdr)/4);
		hdr.DiffServices = 0;
		hdr.TotalLength = htons(sizeof(hdr) + total_length);
		hdr.Identifcation = 0;	// TODO: WTF is this?
		hdr.FragmentInfo = htons(0);
		hdr.TTL = 18;	// TODO: Progammable TTL
		hdr.Protocol = proto;
		hdr.HeaderChecksum = 0;
		memcpy(hdr.SrcAddr, Src, 4);
		memcpy(hdr.DstAddr, Dst, 4);
		
		hdr.HeaderChecksum = htons( IP_Checksum(IP_CHECKSUM_START, sizeof(hdr), &hdr) );
		
		out_buflens[0] = sizeof(hdr);
		out_bufs[0] = &hdr;
		
		// TODO: Don't hard-code MAC addresses
		Link_Send(IfNum, BLOB(HOST_MAC), BLOB(TEST_MAC), ETHER_PROTO_IPV4,
			1+BufCount, out_buflens, out_bufs);
	}
	else {
		TEST_WARN("Invalid AF(%i) in IP_Send", AF);
	}
}

bool IP_Pkt_Check(size_t len, const void *data, size_t *ofs_out, int AF, const void *Src, const void *Dst, uint8_t proto)
{
	size_t	ofs;
	if( AF == 4 ) {
		if( !Link_Pkt_Check(len, data, &ofs, BLOB(TEST_MAC), BLOB(HOST_MAC), ETHER_PROTO_IPV4) )
			return false;
	
		tIPv4Hdr hdr;
		memcpy(&hdr, (const uint8_t*)data + ofs, sizeof(hdr));
		TEST_ASSERT_REL(hdr.VerLen >> 4, ==, 4);
		TEST_ASSERT_REL(IP_Checksum(IP_CHECKSUM_START, sizeof(hdr), &hdr), ==, 0);
		
		TEST_ASSERT_REL(ntohs(hdr.TotalLength), <=, len - ofs);
		TEST_ASSERT_REL(ntohs(hdr.FragmentInfo), ==, 0);
		
		TEST_ASSERT_REL(hdr.TTL, >, 1);	// >1 because there's no intervening hops
		TEST_ASSERT_REL(hdr.Protocol, ==, proto);

		if(Src)	TEST_ASSERT( memcmp(hdr.SrcAddr, Src, 4) == 0 );
		if(Dst)	TEST_ASSERT( memcmp(hdr.DstAddr, Dst, 4) == 0 );
	
		*ofs_out = ofs + (hdr.VerLen & 0xF) * 4;
		return true;
	}
	else {
		TEST_WARN("Invalid AF(%i) in IP_Pkt_Check", AF);
		return false;
	}
}


/*
 */
#include "common.h"
#include "tcp.h"
#include "ip.h"
#include "test.h"	// TEST_ASSERT
#include <string.h>

typedef struct {
	uint16_t	SPort;
	uint16_t	DPort;
	uint32_t	Seq;
	uint32_t	Ack;
	uint8_t 	DataOfs;
	uint8_t 	Flags;
	uint16_t	Window;
	uint16_t	Checksum;
	uint16_t	UrgPtr;
} __attribute__((packed)) tTCPHeader;

// === CODE ===
void TCP_Send(int IF, int AF, const void *IP, short sport, short dport,
	uint32_t seq, uint32_t ack, uint8_t flags, uint16_t window,
	size_t data_len, const void *data
	)
{
	tTCPHeader	hdr;
	hdr.SPort = htons(sport);
	hdr.DPort = htons(dport);
	hdr.Seq = htonl(seq);
	hdr.Ack = htonl(ack);
	hdr.DataOfs = sizeof(hdr)/4;
	hdr.Flags = flags;
	hdr.Window = htons(window);
	hdr.Checksum = htons(0);
	hdr.UrgPtr = htons(0);

	uint16_t	checksum = IP_CHECKSUM_START;
	checksum = IP_Checksum(checksum, sizeof(hdr), &hdr);
	checksum = IP_Checksum(checksum, data_len, data);
	hdr.Checksum = htons( checksum );

	size_t	buflens[] = {sizeof(hdr), data_len};
	const void *bufs[] = {&hdr, data};
	IP_Send(IF, AF, BLOB(HOST_IP), IP, IPPROTO_TCP, 2, buflens, bufs);
}

bool TCP_Pkt_Check(size_t len, const void *data, size_t *out_ofs, int AF, const void *IP, short sport, short dport,
	uint8_t flags)
{
	size_t	ofs;
	if( !IP_Pkt_Check(len, data, &ofs, AF, IP, BLOB(HOST_IP), IPPROTO_TCP) )
		return false;
	
	tTCPHeader	hdr;
	TEST_ASSERT_REL(len - ofs, >=, sizeof(hdr));	
	memcpy(&hdr, (char*)data + ofs, sizeof(hdr));
	
	TEST_ASSERT_REL( ntohs(hdr.SPort), ==, sport );
	TEST_ASSERT_REL( ntohs(hdr.DPort), ==, dport );
	// TODO: Checks on Seq/Ack
	TEST_ASSERT_REL( hdr.Flags, ==, flags);

	uint16_t	real_cksum = htons(hdr.Checksum);
	hdr.Checksum = 0;
	uint16_t	calc_cksum = IP_CHECKSUM_START;
	calc_cksum = IP_Checksum(calc_cksum, sizeof(hdr), &hdr);
	calc_cksum = IP_Checksum(calc_cksum, len - ofs - sizeof(hdr), (char*)data+ofs+sizeof(hdr));
	TEST_ASSERT_REL( real_cksum, ==, calc_cksum );
	
	*out_ofs = ofs + sizeof(hdr);
	return true;
}


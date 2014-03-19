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
uint16_t TCP_int_GetPseudoHeader(int AF, const void *SrcAddr, const void *DstAddr, uint8_t pctl, size_t Len)
{
	if( AF == 4 ) {
		uint8_t	phdr[12];
		memcpy(phdr+0, SrcAddr, 4);
		memcpy(phdr+4, DstAddr, 4);
		phdr[8] = 0;
		phdr[9] = pctl;
		*(uint16_t*)(phdr+10) = htons(Len);
		
		//test_trace_hexdump("TCP IPv4 PHdr", phdr, sizeof(phdr));
		
		return IP_Checksum(IP_CHECKSUM_START, 12, phdr);
	}
	else {
		TEST_WARN("TCP unknown AF %i", AF);
		return 0;
	}
}

void TCP_SendC(const tTCPConn *Conn, uint8_t flags, size_t data_len, const void *data)
{
	TCP_Send(Conn->IFNum, Conn->AF, Conn->RAddr, Conn->LPort, Conn->RPort,
		Conn->LSeq, Conn->RSeq, flags, Conn->Window, data_len, data);
}
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
	hdr.DataOfs = (sizeof(hdr)/4) << 4;
	hdr.Flags = flags;
	hdr.Window = htons(window);
	hdr.Checksum = htons(0);
	hdr.UrgPtr = htons(0);

	uint16_t	checksum;
	checksum = TCP_int_GetPseudoHeader(AF, BLOB(HOST_IP), IP, IPPROTO_TCP, sizeof(hdr)+data_len);
	checksum = IP_Checksum(checksum, sizeof(hdr), &hdr);
	checksum = IP_Checksum(checksum, data_len, data);
	hdr.Checksum = htons( checksum );

	size_t	buflens[] = {sizeof(hdr), data_len};
	const void *bufs[] = {&hdr, data};
	IP_Send(IF, AF, BLOB(HOST_IP), IP, IPPROTO_TCP, 2, buflens, bufs);
}

struct {
	bool	Seq;
	bool	Ack;
	bool	SPort;
}	gTCP_Skips;

void TCP_SkipCheck_Seq(bool Skip) {
	gTCP_Skips.Seq = Skip;
}


bool TCP_Pkt_CheckC(size_t len, const void *data, size_t *out_ofs, size_t *len_out,
	const tTCPConn *conn, uint8_t flags)
{
	return TCP_Pkt_Check(len, data, out_ofs, len_out,
		conn->AF, conn->RAddr, conn->RPort, conn->LPort, conn->RSeq, conn->LSeq, flags
		);
}

bool TCP_Pkt_Check(size_t len, const void *data, size_t *out_ofs, size_t *len_out,
	int AF, const void *IP, short sport, short dport,
	uint32_t seq, uint32_t ack, uint8_t flags)
{
	size_t	ofs, rlen;
	if( !IP_Pkt_Check(len, data, &ofs, &rlen, AF, IP, BLOB(HOST_IP), IPPROTO_TCP) )
		return false;
	
	tTCPHeader	hdr;
	TEST_ASSERT_REL(rlen, >=, sizeof(hdr));	
	memcpy(&hdr, (char*)data + ofs, sizeof(hdr));
	
	TEST_ASSERT_REL( hdr.DataOfs >> 4, >=, sizeof(hdr)/4 );
	if( !gTCP_Skips.SPort )	TEST_ASSERT_REL( ntohs(hdr.SPort), ==, sport );
	TEST_ASSERT_REL( ntohs(hdr.DPort), ==, dport );
	if( !gTCP_Skips.Seq )	TEST_ASSERT_REL( ntohl(hdr.Seq), ==, seq );
	if( flags & TCP_ACK )	TEST_ASSERT_REL( ntohl(hdr.Ack), ==, ack );
	TEST_ASSERT_REL( hdr.Flags, ==, flags);

	uint16_t	real_cksum = htons(hdr.Checksum);
	hdr.Checksum = 0;
	uint16_t	calc_cksum;
	calc_cksum = TCP_int_GetPseudoHeader(AF, IP, BLOB(HOST_IP), IPPROTO_TCP, rlen);
	calc_cksum = IP_Checksum(calc_cksum, sizeof(hdr), &hdr);
	calc_cksum = IP_Checksum(calc_cksum, rlen - sizeof(hdr), (char*)data+ofs+sizeof(hdr));
	TEST_ASSERT_REL( real_cksum, ==, calc_cksum );

	memset(&gTCP_Skips, 0, sizeof(gTCP_Skips));

	*out_ofs = ofs + sizeof(hdr);
	*len_out = rlen - sizeof(hdr);
	return true;
}

uint32_t TCP_Pkt_GetSeq(size_t len, const void *data, int AF)
{
	size_t	ofs, rlen;
	IP_Pkt_Check(len, data, &ofs, &rlen, AF, NULL, NULL, IPPROTO_TCP);
	
	tTCPHeader	hdr;
	memcpy(&hdr, (char*)data + ofs, sizeof(hdr));
	return ntohl(hdr.Seq);
}


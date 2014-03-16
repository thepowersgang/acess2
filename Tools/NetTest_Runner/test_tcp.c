/*
 */
#include "test.h"
#include "tests.h"
#include "net.h"
#include "stack.h"
#include "arp.h"
#include "tcp.h"

bool Test_TCP_Basic(void)
{
	TEST_SETNAME(__func__);
	size_t	rxlen, ofs;
	char rxbuf[MTU];
	
	// 1. Test connection to closed port
	uint32_t	seq_tx = 0x1000;
	uint32_t	seq_exp = 0;
	TCP_Send(0, 4, BLOB(TEST_IP), 1234, 80, seq_tx, seq_exp, TCP_SYN, 0x1000, 0, NULL);
	
	TEST_ASSERT( rxlen = Net_Receive(0, sizeof(rxbuf), rxbuf, 1000) );
	TEST_ASSERT( TCP_Pkt_Check(rxlen, rxbuf, &ofs, 4, BLOB(TEST_IP), 80, 1234, TCP_RST) );
	TEST_ASSERT_REL(ofs, ==, rxlen);
	
	return true;
}

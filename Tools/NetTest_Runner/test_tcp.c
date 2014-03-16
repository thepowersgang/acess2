/*
 * Acess2 Network Stack Tester
 * - By John Hodge (thePowersGang)
 *
 * test_tcp.c
 * - Tests for the behavior of the "Transmission Control Protocol"
 */
#include "test.h"
#include "tests.h"
#include "net.h"
#include "stack.h"
#include "arp.h"
#include "tcp.h"

#define TEST_ASSERT_rx()	TEST_ASSERT( rxlen = Net_Receive(0, sizeof(rxbuf), rxbuf, ERX_TIMEOUT) )
#define TEST_ASSERT_no_rx()	TEST_ASSERT( Net_Receive(0, sizeof(rxbuf), rxbuf, NRX_TIMEOUT) == 0 )

bool Test_TCP_Basic(void)
{
	TEST_SETNAME(__func__);
	size_t	rxlen, ofs;
	char rxbuf[MTU];
	const int	ERX_TIMEOUT = 1000;	// Expect RX timeout (timeout=failure)
	const int	NRX_TIMEOUT = 250;	// Not expect RX timeout (timeout=success)
	
	const char testblob[] = "HelloWorld, this is some random testing data for TCP\xFF\x00\x66\x12\x12";
	const size_t	testblob_len = sizeof(testblob);
	
	// 1. Test packets to closed port
	// > RFC793 Pg.65
	const uint16_t	our_window = 0x1000;
	uint32_t	seq_tx = 0x1000;
	uint32_t	seq_exp = 0x33456;
	
	// 1.1. Send SYN packet
	TCP_Send(0, 4, BLOB(TEST_IP), 1234, 80, seq_tx, seq_exp, TCP_SYN, 0x1000, testblob_len, testblob);
	// Expect a TCP_RST|TCP_ACK with SEQ=0,ACK=SEQ+LEN
	TEST_ASSERT_rx();
	TEST_ASSERT( TCP_Pkt_Check(rxlen, rxbuf, &ofs, 4, BLOB(TEST_IP), 80, 1234,
		0, seq_tx+testblob_len, TCP_RST|TCP_ACK) );
	TEST_ASSERT_REL(ofs, ==, rxlen);
	
	// 1.2. Send a SYN,ACK packet
	TCP_Send(0, 4, BLOB(TEST_IP), 1234, 80, seq_tx, seq_exp, TCP_SYN|TCP_ACK, 0x1000, 0, NULL);
	// Expect a TCP_RST with SEQ=ACK
	TEST_ASSERT_rx();
	TEST_ASSERT( TCP_Pkt_Check(rxlen, rxbuf, &ofs, 4, BLOB(TEST_IP), 80, 1234, seq_exp, seq_tx+0, TCP_RST) );
	TEST_ASSERT_REL(ofs, ==, rxlen);
	
	// 1.3. Send a RST packet
	TCP_Send(0, 4, BLOB(TEST_IP), 1234, 80, seq_tx, seq_exp, TCP_RST, 0x1000, 0, NULL);
	// Expect nothing
	TEST_ASSERT_no_rx();
	
	// 1.3. Send a RST,ACK packet
	TCP_Send(0, 4, BLOB(TEST_IP), 1234, 80, seq_tx, seq_exp, TCP_RST|TCP_ACK, 0x1000, 0, NULL);
	// Expect nothing
	TEST_ASSERT_no_rx();

	
	// 2. Establishing connection with a server
	const int server_port = 1024;
	const int local_port = 11234;
	Stack_SendCommand("tcp_echo_server %i", server_port);

	// >>> STATE: LISTEN

	// 2.1. Send RST
	TCP_Send(0, 4, BLOB(TEST_IP), local_port, server_port, seq_tx, seq_exp, TCP_RST, our_window, 0, NULL);
	// - Expect nothing
	TEST_ASSERT_no_rx();
	// 2.2. Send ACK
	TCP_Send(0, 4, BLOB(TEST_IP), local_port, server_port, seq_tx, seq_exp, TCP_ACK, our_window, 0, NULL);
	// - Expect RST
	TEST_ASSERT_rx();
	TEST_ASSERT( TCP_Pkt_Check(rxlen, rxbuf, &ofs, 4, BLOB(TEST_IP),
		server_port, local_port, seq_exp, seq_tx+0, TCP_RST) );

	// 2.3. Begin hanshake (SYN)
	// TODO: "If the SYN bit is set, check the security."
	TCP_Send(0, 4, BLOB(TEST_IP), local_port, server_port, seq_tx, 0, TCP_SYN, our_window, 0, NULL);
	// - Expect SYN,ACK with ACK == SEQ+1
	TEST_ASSERT_rx();
	TCP_SkipCheck_Seq(true);
	TEST_ASSERT( TCP_Pkt_Check(rxlen, rxbuf, &ofs, 4, BLOB(TEST_IP),
		server_port, local_port, 0, seq_tx+1, TCP_SYN|TCP_ACK) );
	seq_exp = TCP_Pkt_GetSeq(rxlen, rxbuf, 4);

	// >>> STATE: SYN-RECEIVED
	// TODO: Test other transitions from SYN-RECEIVED
		
	// 2.4. Complete handshake, TCP ACK
	seq_exp ++;
	seq_tx ++;
	TCP_Send(0,4,BLOB(TEST_IP), local_port, server_port, seq_tx, seq_exp, TCP_ACK, our_window, 0, NULL);
	// - Expect nothing
	TEST_ASSERT_no_rx();

	// >>> STATE: ESTABLISHED
	
	// 2.5. Send data
	
	return true;
}

bool Test_TCP_SYN_RECEIVED(void)
{
	// 1. Get into SYN-RECEIVED
	
	// 2. Send various non-ACK packets
	return false;
}

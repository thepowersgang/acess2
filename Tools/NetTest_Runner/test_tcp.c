/*
 * Acess2 Network Stack Tester
 * - By John Hodge (thePowersGang)
 *
 * test_tcp.c
 * - Tests for the behavior of the "Transmission Control Protocol"
 * - These tests are written off RFC793
 */
#include "test.h"
#include "tests.h"
#include "net.h"
#include "stack.h"
#include "arp.h"
#include "tcp.h"
#include <string.h>

#define TEST_TIMERS	0

static const int	ERX_TIMEOUT = 1000;	// Expect RX timeout (timeout=failure)
static const int	NRX_TIMEOUT = 250;	// Not expect RX timeout (timeout=success)
static const int	RETX_TIMEOUT = 1000;	// OS PARAM - Retransmit timeout
static const int	LOST_TIMEOUT = 1000;	// OS PARAM - Time before sending an ACK 
static const int	DACK_TIMEOUT = 500;	// OS PARAM - Timeout for delayed ACKs
static const size_t	DACK_BYTES = 4096;	// OS PARAM - Threshold for delayed ACKs

bool Test_TCP_Basic(void)
{
	TEST_HEADER;

	tTCPConn	testconn = {
		.IFNum = 0, .AF = 4,
		.RAddr = BLOB(TEST_IP),
		.LAddr = BLOB(HOST_IP),
		.RPort = 80,
		.LPort = 11200,
		.Window = 0x1000,
		.LSeq = 0x1000,
		.RSeq = 0,
	};

	const char testblob[] = "HelloWorld, this is some random testing data for TCP\xFF\x00\x66\x12\x12.";
	const size_t	testblob_len = sizeof(testblob);

	// TODO: Check checksum failures	

	// 1. Test packets to closed port
	// > RFC793 Pg.65
	
	// 1.1. Send SYN packet
	TEST_STEP("1.1. Send SYN packet to CLOSED");
	TCP_SendC(&testconn, TCP_SYN, testblob_len, testblob);
	testconn.RSeq = 0;
	testconn.LSeq += testblob_len;
	// Expect RST,ACK with SEQ=0
	TEST_ASSERT_rx();
	TEST_ASSERT( TCP_Pkt_CheckC(rxlen, rxbuf, &ofs, &len, &testconn, TCP_RST|TCP_ACK) );
	TEST_ASSERT_REL(ofs, ==, rxlen);
	
	// 1.2. Send a SYN,ACK packet
	TEST_STEP("1.2. Send SYN,ACK packet to CLOSED");
	testconn.RSeq = 12345;
	TCP_SendC(&testconn, TCP_SYN|TCP_ACK, 0, NULL);
	// Expect a TCP_RST with SEQ=ACK
	TEST_ASSERT_rx();
	TEST_ASSERT( TCP_Pkt_CheckC(rxlen, rxbuf, &ofs, &len, &testconn, TCP_RST) );
	TEST_ASSERT_REL(ofs, ==, rxlen);
	testconn.LSeq ++;
	
	// 1.3. Send a RST packet
	TEST_STEP("1.2. Send RST packet to CLOSED");
	TCP_SendC(&testconn, TCP_RST, 0, NULL);
	// Expect nothing
	TEST_ASSERT_no_rx();
	testconn.LSeq ++;
	
	// 1.3. Send a RST,ACK packet
	TCP_SendC(&testconn, TCP_RST|TCP_ACK, 0, NULL);
	// Expect nothing
	TEST_ASSERT_no_rx();
	testconn.LSeq ++;

	
	// 2. Establishing connection with a server
	testconn.RPort = 7;
	testconn.LPort = 11239;
	Stack_SendCommand("tcp_echo_server %i", testconn.RPort);

	// >>> STATE: LISTEN

	// 2.1. Send RST
	TCP_SendC(&testconn, TCP_RST, 0, NULL);
	// - Expect nothing
	TEST_ASSERT_no_rx();
	// 2.2. Send ACK
	TCP_SendC(&testconn, TCP_ACK, 0, NULL);
	// - Expect RST
	TEST_ASSERT_rx();
	TEST_ASSERT( TCP_Pkt_CheckC(rxlen, rxbuf, &ofs, &len, &testconn, TCP_RST) );
	TEST_ASSERT_REL(ofs, ==, rxlen);

	// 2.3. Begin hanshake (SYN)
	// TODO: Test "If the SYN bit is set, check the security."
	TCP_SendC(&testconn, TCP_SYN, 0, NULL);
	testconn.LSeq ++;
	// - Expect SYN,ACK with ACK == SEQ+1
	TEST_ASSERT_rx();
	TCP_SkipCheck_Seq(true);
	TEST_ASSERT( TCP_Pkt_CheckC(rxlen, rxbuf, &ofs, &len, &testconn, TCP_SYN|TCP_ACK) );
	testconn.RSeq = TCP_Pkt_GetSeq(rxlen, rxbuf, testconn.AF) + 1;

	// >>> STATE: SYN-RECEIVED
	// TODO: Test other transitions from SYN-RECEIVED
	
	// 2.4. Complete handshake, TCP ACK
	TCP_SendC(&testconn, TCP_ACK, 0, NULL);
	// - Expect nothing
	TEST_ASSERT_no_rx();

	// >>> STATE: ESTABLISHED
	
	// 2.5. Send data
	TCP_SendC(&testconn, TCP_ACK|TCP_PSH, testblob_len, testblob);
	testconn.LSeq += testblob_len;

	// Expect echoed reponse with ACK
	TEST_ASSERT_rx();
	TEST_ASSERT( TCP_Pkt_CheckC(rxlen, rxbuf, &ofs, &len, &testconn, TCP_ACK|TCP_PSH) );
	TEST_ASSERT_REL( len, ==, testblob_len );
	TEST_ASSERT( memcmp(rxbuf + ofs, testblob, testblob_len) == 0 );
	testconn.RSeq += testblob_len;

	// Send something short
	const char testblob2[] = "test blob two.";
	const size_t	testblob2_len = sizeof(testblob2);
	TCP_SendC(&testconn, TCP_ACK|TCP_PSH, testblob2_len, testblob2);
	testconn.LSeq += testblob2_len;
	// Expect response with data and ACK
	TEST_ASSERT_rx();
	TEST_ASSERT( TCP_Pkt_CheckC(rxlen, rxbuf, &ofs, &len, &testconn, TCP_ACK|TCP_PSH) );
	TEST_ASSERT_REL( len, ==, testblob2_len );
	TEST_ASSERT( memcmp(rxbuf + ofs, testblob2, testblob2_len) == 0 );
	
	// Wait for just under retransmit time, expecting nothing
	#if TEST_TIMERS
	TEST_ASSERT( 0 == Net_Receive(0, sizeof(rxbuf), rxbuf, RETX_TIMEOUT-100) );
	// Now expect the previous message
	TEST_ASSERT_rx();
	TEST_ASSERT( TCP_Pkt_CheckC(rxlen, rxbuf, &ofs, &len, &testconn, TCP_ACK|TCP_PSH) );
	TEST_ASSERT_REL( len, ==, testblob2_len );
	TEST_ASSERT( memcmp(rxbuf + ofs, testblob2, testblob2_len) == 0 );
	#else
	TEST_WARN("Not testing retransmit timer");
	#endif
	testconn.RSeq += testblob2_len;
	
	// Send explicit acknowledgement
	TCP_SendC(&testconn, TCP_ACK, 0, NULL);
	TEST_ASSERT_no_rx();
	
	// TODO: Test delayed ACKs (Timeout and data)
	// > Requires inhibiting the server's echo response?
	
	// === Test out-of-order packets ===
	testconn.LSeq += testblob2_len;	// raise sequence number
	TCP_SendC(&testconn, TCP_ACK|TCP_PSH, testblob_len, testblob);
	// - previous data has not been sent, expect no response for ()
	// TODO: Should this ACK be delayed?
	//TEST_ASSERT_no_rx();
	// - Expect an ACK of the highest received packet
	testconn.LSeq -= testblob2_len;
	TEST_ASSERT_rx();
	TEST_ASSERT( TCP_Pkt_CheckC(rxlen, rxbuf, &ofs, &len, &testconn, TCP_ACK) );
	TEST_ASSERT_REL( len, ==, 0 );
	// - Send missing data
	TCP_SendC(&testconn, TCP_ACK, testblob2_len, testblob2);
	testconn.LSeq += testblob_len+testblob2_len;	// raise sequence number
	// - Expect echo response with all sent data
	TEST_ASSERT_rx();
	TEST_ASSERT( TCP_Pkt_CheckC(rxlen, rxbuf, &ofs, &len, &testconn, TCP_ACK|TCP_PSH) );
	TEST_ASSERT_REL( len, ==, testblob_len+testblob2_len );
	TEST_ASSERT( memcmp(rxbuf + ofs, testblob2, testblob2_len) == 0 );
	TEST_ASSERT( memcmp(rxbuf + ofs+testblob2_len, testblob, testblob_len) == 0 );
	testconn.RSeq += len;
	
	// 2.6. Close connection (TCP FIN)
	TCP_SendC(&testconn, TCP_ACK|TCP_FIN, 0, NULL);
	testconn.LSeq ++;	// Empty = 1 byte
	// Expect ACK? (Does Acess do delayed ACKs here?)
	TEST_ASSERT_rx();
	TEST_ASSERT( TCP_Pkt_CheckC(rxlen, rxbuf, &ofs, &len, &testconn, TCP_ACK) );
	TEST_ASSERT_REL( len, ==, 0 );
	// >>> STATE: CLOSE WAIT
	
	// Expect FIN
	TEST_ASSERT_rx();
	TEST_ASSERT( TCP_Pkt_CheckC(rxlen, rxbuf, &ofs, &len, &testconn, TCP_FIN) );
	TEST_ASSERT_REL( len, ==, 0 );
	
	// >>> STATE: LAST-ACK

	// 2.7 Send ACK of FIN
	TCP_SendC(&testconn, TCP_ACK, 0, NULL);
	// Expect no response
	TEST_ASSERT_no_rx();
	
	// >>> STATE: CLOSED
	
	return true;
}

bool Test_TCP_int_OpenConnection(tTCPConn *Conn)
{
	RX_HEADER;
	// >> SYN
	TCP_SendC(Conn, TCP_SYN, 0, NULL);
	Conn->LSeq ++;
	TEST_ASSERT_rx();
	// << SYN|ACK (save remote sequence number)
	TCP_SkipCheck_Seq(true);
	TEST_ASSERT( TCP_Pkt_CheckC(rxlen, rxbuf, &ofs, &len, Conn, TCP_SYN|TCP_ACK) );
	TEST_ASSERT_REL(len, ==, 0);
	Conn->RSeq = TCP_Pkt_GetSeq(rxlen, rxbuf, Conn->AF) + 1;
	// >> ACK
	TCP_SendC(Conn, TCP_ACK, 0, NULL);
	TEST_ASSERT_no_rx();
	return true;
}

#if 0
bool Test_TCP_SYN_RECEIVED(void)
{
	TEST_HEADER;
	
	// 1. Get into SYN-RECEIVED
	TCP_SendC(&testconn, TCP_SYN, 0, NULL);
	
	// 2. Send various non-ACK packets
	return false;
}
#endif

bool Test_TCP_Reset(void)
{
	TEST_HEADER;
	
	tTCPConn	testconn = {
		.IFNum = 0,
		.AF = 4,
		.LAddr = BLOB(HOST_IP),
		.RAddr = BLOB(TEST_IP),
		.LPort = 44359,
		.RPort = 9,
		.LSeq = 0x600,
		.RSeq = 0x600,
		.Window = 128
	};

	Stack_SendCommand("tcp_echo_server %i", testconn.RPort);

	// 1. Response in listen-based SYN-RECEIVED
	// >> SYN
	TCP_SendC(&testconn, TCP_SYN, 0, NULL);
	testconn.LSeq ++;
	// << SYN|ACK :: Now in SYN-RECEIVED
	TEST_ASSERT_rx();
	TCP_SkipCheck_Seq(true);
	TEST_ASSERT( TCP_Pkt_CheckC(rxlen, rxbuf, &ofs, &len, &testconn, TCP_SYN|TCP_ACK) );
	TEST_ASSERT_REL(len, ==, 0);
	testconn.RSeq = TCP_Pkt_GetSeq(rxlen, rxbuf, testconn.AF) + 1;
	// >> RST (not ACK)
	TCP_SendC(&testconn, TCP_RST, 0, NULL);
	// << nothing (connection should now be dead)
	TEST_ASSERT_no_rx();
	// >> ACK (this should be to a closed conneciton, see LISTEN[ACK] above)
	TCP_SendC(&testconn, TCP_ACK, 0, NULL);
	// << RST
	TEST_ASSERT_rx();
	TEST_ASSERT( TCP_Pkt_CheckC(rxlen, rxbuf, &ofs, &len, &testconn, TCP_RST) );
	TEST_ASSERT_REL(len, ==, 0);
	
	// 2. Response in open-based SYN-RECEIVED? (What is that?)
	TEST_WARN("TODO: RFC793 pg70 mentions OPEN-based SYN-RECEIVED");
	
	testconn.LPort += 1234;
	// ESTABLISHED[RST] - RFC793:Pg70
	// 2. Response in ESTABLISHED 
	TEST_ASSERT( Test_TCP_int_OpenConnection(&testconn) );
	// >> RST
	TCP_SendC(&testconn, TCP_RST, 0, NULL);
	// << no response, connection closed
	TEST_ASSERT_no_rx();
	// >> ACK (LISTEN[ACK])
	TCP_SendC(&testconn, TCP_ACK, 0, NULL);
	// << RST
	TEST_ASSERT_rx();
	TEST_ASSERT( TCP_Pkt_CheckC(rxlen, rxbuf, &ofs, &len, &testconn, TCP_RST) );
	TEST_ASSERT_REL(len, ==, 0);
	
	return true;
}

bool Test_TCP_WindowSizes(void)
{
	TEST_HEADER;
	tTCPConn	testconn = {
		.IFNum = 0,
		.AF = 4,
		.LAddr = BLOB(HOST_IP),
		.RAddr = BLOB(TEST_IP),
		.LPort = 44359,
		.RPort = 9,
		.LSeq = 0x600,
		.RSeq = 0x600,
		.Window = 128
	};
	char	testdata[152];
	memset(testdata, 0xAB, sizeof(testdata));
	
	Stack_SendCommand("tcp_echo_server %i", testconn.RPort);
	// > Open Connection
	TEST_ASSERT( Test_TCP_int_OpenConnection(&testconn) );
	
	// 1. Test remote respecting our transmission window (>=1 byte)
	// > Send more data than our RX window
	TCP_SendC(&testconn, TCP_PSH, sizeof(testdata), testdata);
	testconn.LSeq += sizeof(testdata);
	// Expect our RX window back
	TEST_ASSERT_rx();
	TEST_ASSERT( TCP_Pkt_CheckC(rxlen, rxbuf, &ofs, &len, &testconn, TCP_ACK|TCP_PSH) );
	TEST_ASSERT_REL(len, ==, testconn.Window);
	testconn.RSeq += len;
	// > Send ACK and reduce window to 1 byte
	testconn.Window = 1;
	TCP_SendC(&testconn, TCP_ACK, 0, NULL);
	testconn.LSeq += sizeof(testdata);
	// > Expect 1 byte back
	TEST_ASSERT_rx();
	TEST_ASSERT( TCP_Pkt_CheckC(rxlen, rxbuf, &ofs, &len, &testconn, TCP_ACK|TCP_PSH) );
	TEST_ASSERT_REL(len, ==, 1);
	testconn.RSeq += len;
	// 2. Test remote handling our window being 0 (should only send ACKs)
	// TODO: 
	// 3. Test that remote drops data outside of its RX window
	// 3.1. Ensure that data that wraps the end of the RX window is handled correctly
	// TODO: 
	return false;
}

// RFC793 pg41
bool Test_TCP_Retransmit(void)
{
	TEST_HEADER;
	tTCPConn	testconn = {
		.IFNum = 0,
		.AF = 4,
		.LAddr = BLOB(HOST_IP),
		.RAddr = BLOB(TEST_IP),
		.LPort = 44359,
		.RPort = 9,
		.LSeq = 0x600,
		.RSeq = 0x600,
		.Window = 128
	};
	char	testdata[128];
	memset(testdata, 0xAB, sizeof(testdata));

	TEST_STEP("1. Open and connect to TCP server");
	Stack_SendCommand("tcp_echo_server %i", testconn.RPort);
	TEST_ASSERT( Test_TCP_int_OpenConnection(&testconn) );
	
	
	TEST_STEP("2. Send data and expect it to be echoed");
	TCP_SendC(&testconn, TCP_PSH, sizeof(testdata), testdata);
	testconn.LSeq += sizeof(testdata);
	TEST_ASSERT_rx();
	TEST_ASSERT( TCP_Pkt_CheckC(rxlen, rxbuf, &ofs, &len, &testconn, TCP_ACK|TCP_PSH) );
	
	TEST_STEP("3. Expect nothing for TCP_RTX_TIMEOUT_1");
	TEST_ASSERT( Net_Receive(0, sizeof(rxbuf), rxbuf, RETX_TIMEOUT-100) == 0 );
	
	TEST_STEP("4. Expect a retransmit");
	TEST_ASSERT_rx();
	
	return false;
}

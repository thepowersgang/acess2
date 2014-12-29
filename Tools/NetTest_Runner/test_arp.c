/*
 */
#include "test.h"
#include "tests.h"
#include "net.h"
#include "stack.h"
#include "arp.h"

static const int	ERX_TIMEOUT = 1000;	// Expect RX timeout (timeout=failure)
static const int	NRX_TIMEOUT = 250;	// Not expect RX timeout (timeout=success)

bool Test_ARP_Basic(void)
{
	TEST_HEADER;
	
	// Request test machine's IP
	ARP_SendRequest(0, BLOB(TEST_IP));
	TEST_ASSERT_rx();
	TEST_ASSERT( ARP_Pkt_IsResponse(rxlen, rxbuf, BLOB(TEST_IP), BLOB(TEST_MAC)) );

	// Request host machine's IP
	ARP_SendRequest(0, BLOB(HOST_IP));
	TEST_ASSERT_no_rx();

	#if 0	
	// Ask test machine to request our IP
	Stack_SendCommand("arprequest "HOST_IP_STR);
	TEST_ASSERT_rx();
	TEST_ASSERT( ARP_Pkt_IsRequest(rxlen, rxbuf, HOST_IP) );

	// Respond
	ARP_SendResponse(0, HOST_IP, HOST_MAC);
	
	// Ask test machine to request our IP again (expecting nothing)
	Stack_SendCommand("arprequest "HOST_IP_STR);
	TEST_ASSERT_no_rx();
	#endif
	
	return true;
}

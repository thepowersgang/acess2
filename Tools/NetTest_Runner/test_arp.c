/*
 */
#include "test.h"
#include "tests.h"
#include "net.h"
#include "stack.h"
#include "arp.h"

bool Test_ARP_Basic(void)
{
	TEST_SETNAME(__func__);
	size_t	rxlen;
	char rxbuf[MTU];
	
	// Request test machine's IP
	ARP_SendRequest(0, BLOB(TEST_IP));
	TEST_ASSERT( rxlen = Net_Receive(0, sizeof(rxbuf), rxbuf, 1000) );
	TEST_ASSERT( ARP_Pkt_IsResponse(rxlen, rxbuf, BLOB(TEST_IP), BLOB(TEST_MAC)) );

	// Request host machine's IP
	ARP_SendRequest(0, BLOB(HOST_IP));
	TEST_ASSERT( Net_Receive(0, sizeof(rxbuf), rxbuf, 1000) == 0 );

	#if 0	
	// Ask test machine to request our IP
	Stack_SendCommand("arprequest "HOST_IP_STR);
	TEST_ASSERT( rxlen = Net_Receive(0, sizeof(rxbuf), rxbuf, 1000) );
	TEST_ASSERT( ARP_Pkt_IsRequest(rxlen, rxbuf, HOST_IP) );

	// Respond
	ARP_SendResponse(0, HOST_IP, HOST_MAC);
	
	// Ask test machine to request our IP again (expecting nothing)
	Stack_SendCommand("arprequest "HOST_IP_STR);
	TEST_ASSERT( !Net_Receive(0, sizeof(rxbuf), rxbuf, 1000) );
	#endif
	
	return true;
}

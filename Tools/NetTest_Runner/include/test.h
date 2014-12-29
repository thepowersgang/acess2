/*
 */
#ifndef _TEST_H_
#define _TEST_H_

#include <stddef.h>

#define TEST_SETNAME(name)	test_setname(name)
#define TEST_STEP(name) 	do{}while(0)	//test_setstep(name)
#define TEST_ASSERT(cnd)	do{if(!(cnd)) {test_assertion_fail(__FILE__,__LINE__,"%s",#cnd);return false;}}while(0)
#define TEST_ASSERT_REL(a,r,b)	do{long long a_val=(a),b_val=(b);if(!(a_val r b_val)) {test_assertion_fail(__FILE__,__LINE__,"%s(0x%llx)%s%s(0x%llx)",#a,a_val,#r,#b,b_val);return false;}}while(0)
#define TEST_WARN(msg...)	test_message(__FILE__,__LINE__,msg)

extern void	test_setname(const char *name);
extern void	test_message(const char *filename, int line, const char *msg, ...);
extern void	test_assertion_fail(const char *filename, int line, const char *test, ...);
extern void	test_trace(const char *msg, ...);
extern void	test_trace_hexdump(const char *hdr, const void *data, size_t len);

// Some helpful macros
// - They require some names to be present
#define RX_HEADER \
	size_t	rxlen, ofs, len; \
	do { ofs = 0; ofs = ofs; len = 0; len = len; } while(0);\
	char rxbuf[MTU]
#define TEST_HEADER \
	TEST_SETNAME(__func__);\
	RX_HEADER

#define TEST_ASSERT_rx()	TEST_ASSERT( rxlen = Net_Receive(0, sizeof(rxbuf), rxbuf, ERX_TIMEOUT) )
#define TEST_ASSERT_no_rx()	TEST_ASSERT( Net_Receive(0, sizeof(rxbuf), rxbuf, NRX_TIMEOUT) == 0 )

#endif


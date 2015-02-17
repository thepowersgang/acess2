/*
 * Acess2 C Library (Test)
 * - By John Hodge (thePowersGang)
 *
 * TEST_strtoi.c
 * - Tests for strtoi.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <string.h>	// strerror

#define STR_(v)	#v
#define STR(v)	STR_(v)
#define TST(t, class, base, val, exp, fmt, ofs, exp_errno) do {\
	const char *in = val;\
	char *end;\
	errno = 0;\
	t ret = strto##class(in, &end, base); \
	if( ret != exp ) { \
		fprintf(stderr, "FAIL strto"#class"('%s') != "#exp" (act "fmt")\n", in, ret);\
		exit(1); \
	} \
	if( end != in+ofs ) { \
		fprintf(stderr, "FAIL strto"#class"('%s') returned wrong end: %p (+%zi) instead of %p (+%zi)\n",\
			in,end,end-in,in+ofs,(size_t)ofs);\
		exit(1); \
	} \
	if( exp_errno != errno ) { \
		fprintf(stderr, "FAIL strto"#class"('%s') returned wrong errno, exp '%s', got '%s'\n",\
			in, strerror(exp_errno), strerror(errno));\
		exit(1); \
	} \
}while(0)

#define PRIMEBUF(fmt, val)	buf_len = snprintf(buf, sizeof(buf), fmt, val)

int main(int argc, char *argv[])
{
	char buf[64];
	size_t	buf_len;
	
	// Success cases
	TST(unsigned long, ul, 0, "0x10ec", 0x10ec, "%lx", 2+4, 0);
	TST(unsigned long long, ull, 0, "0xffeed10ec", 0xffeed10ec, "%llx", 2+9, 0);
	TST(unsigned long long, ull, 0, "01234567", 01234567, "%llo", 8, 0);
	TST(unsigned long long, ull, 0, "1234567", 1234567, "%lld", 7, 0);
	TST(long long, ll, 0, "-1", -1, "%lld", 2, 0);	// -1
	TST(long long, ll, 0, "100113", 100113, "%lld", strlen(in), 0);
	TST(long long, ll, 0, "0x101", 0x101, "0x%llx", strlen(in), 0);
	
	// Invalid strings
	TST(unsigned long long, ull, 0, "0x",  0, "%llx", 1, 0);	// Single 0
	TST(unsigned long long, ull, 0, "0xg", 0, "%llx", 1, 0);	// Single 0
	TST(unsigned long long, ull, 0, "-a", 0, "%llx", 0, 0);	// Nothing
	TST(long long, ll, 0, "-a", 0, "%lld", 0, 0);	// Nothing
	TST(long long, ll, 0, "-1aaatg", -1, "%lld", 2, 0);	// -1 (with traling junk)
	TST(long long, ll, 0, "-+1aaatg", 0, "%lld", 0, 0);	// Nothing
	TST(long long, ll, 0, "-  1", 0, "%lld", 0, 0);	// Nothing
	TST(long long, ll, 0, "01278  1", 0127, "%lld", 4, 0);	// 0127 with junk
	
	// Range edges
	PRIMEBUF("0x%llx", ULLONG_MAX);
	TST(unsigned long long, ull, 0, buf, ULLONG_MAX, "0x%llx", buf_len, 0);
	PRIMEBUF("%llu", ULLONG_MAX);
	TST(unsigned long long, ull, 0, buf, ULLONG_MAX, "%llu", buf_len, 0);
	PRIMEBUF("-%llu", (long long)LONG_MAX);
	TST(long, l, 0, buf, -LONG_MAX, "%ld", buf_len, 0);
	
	// Out of range
	// - When the range limit is hit, valid characters should still be consumed (just not used)
	TST(unsigned long long, ull, 0, "0x10000FFFF0000FFFF", ULLONG_MAX, "%llx", strlen(in), ERANGE);
	TST(unsigned long, ul, 0, "0x10000FFFF0000FFFF", ULONG_MAX, "%lx", strlen(in), ERANGE);
	TST(long, l, 0, "0x10000FFFF0000FFFF", LONG_MAX, "%ld", strlen(in), ERANGE);
	TST(long, l, 0, "-0x10000FFFF0000FFFF", LONG_MIN, "%ld", strlen(in), ERANGE);
	if( LONG_MIN < -LONG_MAX )
	{
		// Ensure that if -LONG_MIN is greater than LONG_MAX, that converting it leaves a range error
		PRIMEBUF("%ld", LONG_MIN);
		TST(long, l, 0, buf+1, LONG_MAX, "%ld", buf_len-1, ERANGE);
	}
}

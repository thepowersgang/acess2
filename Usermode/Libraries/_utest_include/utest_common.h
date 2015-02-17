#ifndef _UTEST_COMMON_H_
#define _UTEST_COMMON_H_

#include <stdlib.h>

// --- Test assertions
#define TEST_REL_(_ty, _fmt, exp, rel, have) do { \
	_ty a = (exp);\
	_ty b = (have);\
	if( !(a rel b) ) { \
		fprintf(stderr, "TEST_REL_INT("#exp" {%"_fmt"} "#rel" "#have" {%"_fmt"}) FAILED\n", \
			a, b); \
		return 1; \
	} \
} while(0)
#define TEST_REL_INT(exp, rel, have)	TEST_REL_(int, "i", exp, rel, have)
#define TEST_REL_PTR(exp, rel, have)	TEST_REL_(const void*, "p", exp, rel, have)

// -- Header hooks (allowing inclusion of general headers)
#define SYSCALL(rt, name)	rt name() { fprintf(stderr, "BUG: Calling syscall '"#name"' in unit test\n"); exit(2); }

#endif


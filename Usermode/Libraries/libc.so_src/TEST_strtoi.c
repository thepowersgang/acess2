/*
 * Acess2 C Library (Test)
 * - By John Hodge (thePowersGang)
 *
 * TEST_strtoi.c
 * - Tests for strtoi.c
 */
#include <stdio.h>

#define TST(t, class, base, val, exp, fmt) do {\
	t ret = strto##class(#val, NULL, base); \
	if( ret != exp ) \
		printf("FAIL strto"#class"('"#val"') != "#val" (act 0x"fmt")\n", ret);\
}while(0)

int main(int argc, char *argv[])
{
	TST(unsigned long, ul, 0, 0x10ec, 0x10ec, "%x");
}

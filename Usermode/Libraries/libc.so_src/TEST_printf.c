/*
 * Acess2 C Library (Test)
 * - By John Hodge (thePowersGang)
 *
 * TEST_printf.c
 * - Tests for printf.c
 */
#include <stdio.h>
#include <string.h>
#include <utest_common.h>

#define TST(_name, exp, fmt, val...) do { \
	char buf[64]; \
	snprintf(buf, sizeof(buf), fmt, ##val); \
	if( strcmp(buf, exp) != 0 ) { \
		fprintf(stderr, "FAIL: exp '%s' != got '%s'\n", exp, buf);\
		exit(1); \
	} \
} while(0)

int main(int argc, char *argv[])
{
	TST("None", "Hello World!\n", "Hello World!\n");
	TST("String", "teststring", "%s", "teststring");
	TST("String", "tests",      "%.5s", "teststring");
	TST("String", "     tests", "%10.5s", "teststring");
	TST("String", "tests     ", "%-10.5s", "teststring");
	
	TST("Integer", "1234", "%i", 1234);
	TST("Integer", "1234", "%d", 1234);
	TST("Integer", "1234", "%u", 1234);
	
	TST("Float", "3.141593", "%f", 3.1415926535);
	TST("Float", "10.000000",    "%f", 10.0);
	TST("Float", "-0.000000", "%f", -0.0);
	TST("Float", "3.1415926535", "%.10f", 3.1415926535);
	TST("Float", "3.141593e+00", "%e", 3.1415926535);
	TST("Float", "3.14159", "%g", 3.1415926535);
	TST("Float", "1.000000E+09", "%E", 1000000000.00);
	TST("Float", "0x1p+4", "%a", 16.0);
	TST("Float", "0x1p+10", "%a", 1024.0);
	TST("Float", "0x1.ff8p+9", "%a", 1023.0);
	TST("Float", "0X1.DCD65P+29", "%A", 1000000000.00);
	return 0;
}

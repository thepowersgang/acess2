/*
 * Acess2 C Library (Test)
 * - By John Hodge (thePowersGang)
 *
 * TEST_printf.c
 * - Tests for printf.c
 */
#include <stdio.h>

#define TST(_name, fmt, val) \
	printf(_name" %"fmt" '"#val"': '"fmt"'\n", val)

int main(int argc, char *argv[])
{
	printf("Hello World!\n");
	TST("String", "%s", "teststring");
	TST("String", "%.5s", "teststring");
	TST("String", "%10.5s", "teststring");
	TST("String", "%-10.5s", "teststring");
	
	TST("Integer", "%i", 1234);
	TST("Integer", "%d", 1234);
	TST("Integer", "%u", 1234);
	
	TST("Float", "%f", 3.1414926535);
	TST("Float", "%f", 10.0);
	TST("Float", "%.10f", 3.1414926535);
	TST("Float", "%e", 3.1415926535);
	TST("Float", "%g", 3.1415926535);
	TST("Float", "%E", 1000000000.00);
	TST("Float", "%a", 16.0);
	TST("Float", "%a", 1024.0);
	TST("Float", "%a", 1023.0);
	TST("Float", "%A", 1000000000.00);
	return 0;
}

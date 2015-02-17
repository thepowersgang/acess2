/*
 */
#include <stdio.h>
#include <string.h>
#include <utest_common.h>

int main()
{
	TEST_REL_INT(0, > , strcmp("hello", "world"));
	TEST_REL_INT(0, ==, strcmp("hello", "hello"));
	TEST_REL_INT(0, < , strcmp("wello", "hello"));
	TEST_REL_INT(0, < , strcmp("\xff", "\1"));
	TEST_REL_INT(0, > , strcmp("\1", "\xff"));
	TEST_REL_INT(0, > , strcmp("Hello", "hello"));
	
	TEST_REL_INT(0, ==, strncmp("hello world", "hello", 5));
	
	TEST_REL_INT(0, > , strcasecmp("hello", "world"));
	TEST_REL_INT(0, ==, strcasecmp("hello", "hello"));
	TEST_REL_INT(0, < , strcasecmp("wello", "hello"));
	TEST_REL_INT(0, < , strcasecmp("\xff", "\1"));
	TEST_REL_INT(0, > , strcasecmp("\1", "\xff"));
	TEST_REL_INT(0, ==, strcasecmp("Hello", "hello"));
	TEST_REL_INT(0, ==, strcasecmp("Hello", "Hello"));
	TEST_REL_INT(0, ==, strcasecmp("hellO", "Hello"));
	
	char buf[13];
	memset(buf, 127, sizeof(buf));
	TEST_REL_INT(127, ==, buf[0]);
	TEST_REL_INT(127, ==, buf[4]);
	strncpy(buf, "hello", 4);
	TEST_REL_INT('l', ==, buf[3]);
	TEST_REL_INT(127, ==, buf[4]);
	strncpy(buf, "hello", 8);
	TEST_REL_INT('o', ==, buf[4]);
	TEST_REL_INT('\0', ==, buf[5]);
       	TEST_REL_INT('\0', ==, buf[7]);
       	TEST_REL_INT(127, ==, buf[8]);
	
	memset(buf, 0, 13);
	TEST_REL_INT(0, ==, buf[0]);
	TEST_REL_INT(0, ==, buf[12]);
	
	TEST_REL_PTR(NULL, ==, memchr("\xffhello", 'x', 6));
	
	const char *teststr_foo = "foo";
	TEST_REL_PTR(teststr_foo+0, ==, strchr(teststr_foo, 'f'));
	TEST_REL_PTR(teststr_foo+1, ==, strchr(teststr_foo, 'o'));
	TEST_REL_PTR(teststr_foo+3, ==, strchr(teststr_foo, '\0'));
	TEST_REL_PTR(NULL, ==, strchr(teststr_foo, 'X'));
	TEST_REL_PTR(teststr_foo+0, ==, strrchr(teststr_foo, 'f'));
	TEST_REL_PTR(teststr_foo+2, ==, strrchr(teststr_foo, 'o'));
	TEST_REL_PTR(teststr_foo+3, ==, strrchr(teststr_foo, '\0'));
	TEST_REL_PTR(NULL, ==, strrchr(teststr_foo, 'X'));
}


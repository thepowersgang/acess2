/*
 */
#include <stdio.h>
#include <string.h>

#define ASSERT(cnd)	printf("ASSERT: "#cnd" == %s\n", ((cnd) ? "pass" : "FAIL"))

int main()
{
	ASSERT(strcmp("hello", "world") < 0);
	ASSERT(strcmp("hello", "hello") == 0);
	ASSERT(strcmp("wello", "hello") > 0);
	ASSERT(strcmp("\xff", "\1") > 0);
	ASSERT(strcmp("\1", "\xff") < 0);
	ASSERT(strcmp("Hello", "hello") < 0);
	
	ASSERT(strncmp("hello world", "hello", 5) == 0);
	
	ASSERT(strcasecmp("hello", "world") < 0);
	ASSERT(strcasecmp("hello", "hello") == 0);
	ASSERT(strcasecmp("wello", "hello") > 0);
	ASSERT(strcasecmp("\xff", "\1") > 0);
	ASSERT(strcasecmp("\1", "\xff") < 0);
	ASSERT(strcasecmp("Hello", "hello") == 0);
	ASSERT(strcasecmp("Hello", "Hello") == 0);
	ASSERT(strcasecmp("hellO", "Hello") == 0);
	
	char buf[13];
	memset(buf, 127, sizeof(buf));
	ASSERT(buf[0] == 127);	ASSERT(buf[4] == 127);
	strncpy(buf, "hello", 4);
	ASSERT(buf[3] == 'l');	ASSERT(buf[4] == 127);
	strncpy(buf, "hello", 8);
	ASSERT(buf[4] == 'o');	ASSERT(buf[5] == '\0'); ASSERT(buf[7] == '\0'); ASSERT(buf[8] == 127);
	
	memset(buf, 0, 13);
	ASSERT(buf[0] == 0);	ASSERT(buf[12] == 0);
	
	ASSERT(memchr("\xffhello", 'x', 6) == NULL);
	
	const char *teststr_foo = "foo";
	ASSERT(strchr(teststr_foo, 'f') == teststr_foo+0);
	ASSERT(strchr(teststr_foo, 'o') == teststr_foo+1);
	ASSERT(strchr(teststr_foo, '\0') == teststr_foo+3);
	ASSERT(strchr(teststr_foo, 'X') == NULL);
	ASSERT(strrchr(teststr_foo, 'f') == teststr_foo+0);
	ASSERT(strrchr(teststr_foo, 'o') == teststr_foo+2);
	ASSERT(strrchr(teststr_foo, '\0') == teststr_foo+3);
	ASSERT(strrchr(teststr_foo, 'X') == NULL);
}


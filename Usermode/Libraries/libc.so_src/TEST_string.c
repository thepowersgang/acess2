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
	memset(buf, 0, 13);
	ASSERT(buf[0] == 0);	ASSERT(buf[12] == 0);
	
	ASSERT(memchr("\xffhello", 'x', 6) == NULL);
}


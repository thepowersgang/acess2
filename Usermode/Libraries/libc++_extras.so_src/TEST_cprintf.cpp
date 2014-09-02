/*
 */
#include <cxxextras_printf>
#include <cstdio>

int main()
{
	::cxxextras::cprintf(
		[](const char *str, size_t len){printf("%.*s", (unsigned int)len, str); return len;},
		"%s %i %+#-010x",
		"hello_world",
		1337,
		0x1234565);
	return 0;
}


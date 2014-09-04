/*
 */
#include <cxxextras_printf>
#include <cstdio>
#include <stdexcept>

void my_puts(const char *str, size_t len)
{
	fwrite(str, 1, len, stdout);
}

#define ASSERT_EXCEPTION(expr, Exception) do{bool _ok=false; try { expr; } catch(const Exception& e){_ok=true;}if(!_ok)throw ::std::runtime_error("Didn't throw "#Exception);}while(0)

int main()
{
	printf("Success\n");
	::cxxextras::cprintf(my_puts, "%s %i %+#-010x\n", "hello_world", 1337, 0x1234565);
	
	printf("Too Few\n");
	ASSERT_EXCEPTION( ::cxxextras::cprintf(my_puts, "%s %i %+#-010x\n"), ::cxxextras::cprintf_toofewargs );
	printf("Too Many\n");
	ASSERT_EXCEPTION( ::cxxextras::cprintf(my_puts, "%s\n", "tst", 12345), ::cxxextras::cprintf_toomanyargs );
	
	printf("Bad Format\n");
	ASSERT_EXCEPTION( ::cxxextras::cprintf(my_puts, "%-\n"), ::cxxextras::cprintf_badformat );
	
	return 0;
}


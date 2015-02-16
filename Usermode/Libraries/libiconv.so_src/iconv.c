/*
 */
#include <iconv.h>
#include <acess/sys.h>

// === CODE ===
int SoMain(void)
{
	return 0;
}

iconv_t iconv_open(const char *to, const char *from)
{
	return NULL;
}

size_t iconv(iconv_t cd, const char **inbuf, size_t *inbytesleft, char **outbuf, size_t *outbytesleft)
{
	_SysDebug("WTF are you using iconv for?");
	return 0;
}

int iconv_close(iconv_t cd)
{
	return 0;
}


/*
 */
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>

// === CODE ===
int main(int argc, char *argv[])
{
	return 0;
}

int *Threads_GetErrno(void)
{
	return &errno;
}

// TODO: Move into a helper lib?
extern void itoa(char *buf, uint64_t num, int base, int minLength, char pad)
{
	char fmt[] = "%0ll*x";
	switch(base)
	{
	case  8:	fmt[5] = 'o';	break;
	case 10:	fmt[5] = 'd';	break;
	case 16:	fmt[5] = 'x';	break;
	}
	if(pad != '0') {
		fmt[1] = '%';
		sprintf(buf, fmt+1, minLength, num);
	}
	else {
		sprintf(buf, fmt, minLength, num);
	}
}

int strpos(const char *Str, char Ch)
{
	const char *r = strchr(Str, Ch);
	if(!r)	return -1;
	return r - Str;
}

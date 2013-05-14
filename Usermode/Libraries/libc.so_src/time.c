/*
 * Acess2 C Library
 * - By John Hodge (thePowersGang)
 *
 * time.c
 * - POSIX/stdc time functions
 */
#include <time.h>
#include <acess/sys.h>

clock_t clock(void)
{
	return _SysTimestamp();
}

time_t time(time_t *t)
{
	time_t ret = _SysTimestamp() / 1000;
	if(t)
		*t = ret;
	return ret;
}

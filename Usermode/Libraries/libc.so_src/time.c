/*
 * Acess2 C Library
 * - By John Hodge (thePowersGang)
 *
 * time.c
 * - POSIX/stdc time functions
 */
#include <time.h>
#include <acess/sys.h>
#include <string.h>

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

static struct tm	static_tm;

struct tm *localtime(const time_t *timer)
{
	struct tm *ret = &static_tm;

	// TODO: This breaks on negative timestamps

	int64_t	day = *timer / (1000*60*60*24);
	int64_t	iday = *timer % (1000*60*60*24);	

	ret->tm_sec = (iday / 1000) % 60;;
	ret->tm_min = (iday / (1000*60)) % 60;
	ret->tm_hour = (iday / (1000*60*60));
	ret->tm_year = 0;
	ret->tm_mon = 0;
	ret->tm_mday = 0;
	ret->tm_wday = (day + 6) % 7;	// 1 Jan 2000 was a saturday
	ret->tm_yday = 0;
	ret->tm_isdst = 0;	// Fuck DST
	return ret;
}

static inline size_t MIN(size_t a, size_t b) { return a < b ? a : b; }
static size_t _puts(char * restrict s, size_t maxsize, size_t ofs, const char *str, size_t len)
{
	if( s && ofs < maxsize ) {
		size_t	addlen = MIN(len, maxsize-1-ofs);
		memcpy(s+ofs, str, addlen);
	}
	return len;
}

size_t strftime(char*restrict s, size_t maxsize, const char*restrict format, const struct tm*restrict timeptr)
{
	size_t	ofs = 0;

	while( *format )
	{
		const char *restrict start = format;
		while( *format && *format != '%' )
			format ++;
		if( format != start )
			ofs += _puts(s, maxsize, ofs, start, format-start);
		if( *format == 0 )
			break;
		format ++;
		switch(*format++)
		{
		case 0:	format--;	break;
		case '%':	ofs += _puts(s, maxsize, ofs, format-1, 1);	break;
		case 'd':	// The day of the month as a decimal number (range 01 to 31).
			{
			char tmp[2] = {'0','0'};
			tmp[0] += (timeptr->tm_mday / 10) % 10;
			tmp[1] += timeptr->tm_mday % 10;
			ofs += _puts(s, maxsize, ofs, tmp, 2);
			}
			break;
		default:
			_SysDebug("TODO: strftime('...%%%c...')", format[-1]);
			break;
		}
	}
	
	return ofs;
}

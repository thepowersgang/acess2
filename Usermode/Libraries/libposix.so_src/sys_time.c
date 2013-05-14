/*
 * Acess2 POSIX Emulation
 * - By John Hodge (thePowersGang)
 *
 * sys/time.h
 * - Timing calls
 */
#include <sys/time.h>
#include <acess/sys.h>

// === CODE ===
#if 0
int getitimer(int which, struct itimerval *current_value)
{
	
}

int setitimer(int which, const struct itimerval *new_value, struct itimerval *old_value)
{
	
}
#endif

int gettimeofday(struct timeval *tv, struct timezone *tz)
{
	const int correction_2k_to_unix = 30*365*24*3600;
	long long int ts = _SysTimestamp();
	if( tv )
	{
		tv->tv_sec = ts / 1000 + correction_2k_to_unix;
		tv->tv_usec = (ts % 1000) * 1000;
	}
	if( tz )
	{
		// TODO: Determine timezone?
		tz->tz_minuteswest = 0;
		tz->tz_dsttime = 0;
	}
	return 0;
}

// ifdef _BSD_SOURCE
//int settimeofday(const struct timeval *tv, const struct timezone *tz);

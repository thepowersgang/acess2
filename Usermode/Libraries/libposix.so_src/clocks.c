/*
 * Acess2 POSIX Emulation
 * - By John Hodge (thePowersGang)
 *
 * clocks.c
 * - clock_* functions
 */
#include <acess/sys.h>
#include <time.h>
#include <errno.h>

// === CODE ===
int clock_getres(clockid_t clk_id, struct timespec *res)
{
	switch(clk_id)
	{
	case CLOCK_REALTIME:
	case CLOCK_MONOTONIC:
		if( res ) {
			res->tv_sec = 0;
			res->tv_nsec = 1000*1000;
		}
		return 0;
	}
	errno = EINVAL;
	return -1;
}

int clock_gettime(clockid_t clk_id, struct timespec *tp)
{
	switch(clk_id)
	{
	case CLOCK_REALTIME:
	case CLOCK_MONOTONIC:
		// TODO: True monotonic clock
		if( tp ) {
			int64_t	ts = _SysTimestamp();
			tp->tv_sec = ts / 1000;
			tp->tv_nsec = (ts % 1000) * 1000*1000;
		}
		return 0;
	}
	errno = EINVAL;
	return -1;
}

int clock_settime(clockid_t clk_id, const struct timespec *tp)
{
	errno = EPERM;
	return -1;
}

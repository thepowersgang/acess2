/*
 * Acess2 C Library
 * - By John Hodge (thePowersGang)
 *
 * timeconv.c
 * - Shared User/Kernel time conversion code
 */
#include "timeconv.h"
#include <errno.h>

#define ENABLE_LEAP_SECONDS	1

static const short DAYS_IN[] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
static const short DAYS_BEFORE[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
#define DEC31	365
#define JUN30	181

#if ENABLE_LEAP_SECONDS
// Leap seconds after 2000 (stored as day numbers of application)
// - Leap seconds apply after the last second of said day (23:59:60)
// TODO: Need to encode possible double leap seconds, and negative leap seconds
static int64_t	leap_seconds_after[] = {
	// ls + (year + leap + day + thisleap)
	(5*365+2+DEC31+0),	// Dec 31 2005
	(8*365+2+DEC31+1),	// Dec 31 2008
	(12*365+3+JUN30+1)	// Jun 30 2012
};
static int	n_leap_seconds_after = sizeof(leap_seconds_after)/sizeof(leap_seconds_after[0]);
#endif

static int YearIsLeap(int year) {
	return (year % 4 == 0) - (year % 100 == 0) + (year % 400 == 0);
}
static int DaysUntilYear(int year) {
	return year*365 + (year/400) - (year/100) + (year/4);
}

int64_t seconds_since_y2k(int years_since, int mon, int day, int h, int m, int s)
{
	errno = EINVAL;
	if( !(1 <= mon && mon <= 12) )	return 0;
	if( !(1 <= day && day <= DAYS_IN[mon-1]) )	return 0;
	if( !(0 <= h && h <= 23) )	return 0;
	if( !(0 <= m && m <= 59) )	return 0;
	if( !(0 <= s && s <= 60) )	return 0;

	// Special case check for leap days
	if( mon == 2 && day == 29 )
	{
		if( !YearIsLeap(years_since) )
			return 0;
	}

	// Day
	int64_t days = DaysUntilYear(years_since);
	days += DAYS_BEFORE[mon-1];
	if(mon > 2 && YearIsLeap(years_since))
		days ++;
	days += day;
	
	// Seconds
	int64_t	seconds = h * 60*60 + m * 60;
	seconds += s;
	bool today_has_ls = false;
	#if ENABLE_LEAP_SECONDS
	if( days > 0 )
	{
		for( int i = 0; i < n_leap_seconds_after; i ++ )
		{
			if( days < leap_seconds_after[i] )
				break ;
			if(days > leap_seconds_after[i])
				seconds ++;
			else
				today_has_ls = true;
			// A leap second on this day is handled with s=60
		}
	}

	// Now that we know if this day has a leap second, sanity check leap seconds
	if( s == 60 )
	{
		if( !today_has_ls || !(h == 23 && m == 59) )
			return 0;
	}
	#else
	if( s == 60 )	return 0;
	#endif

	errno = 0;
	
	return days * 24*60*60 + seconds;
}

// returns number of days
int64_t get_days_since_y2k(int64_t ts, int *h, int *m, int *s)
{
	#if ENABLE_LEAP_SECONDS
	// Calculate leap second count
	 int	n_leap = 0;
	bool 	is_ls = false;
	if( ts > 0 )
	{
		for( int i = 0; i < n_leap_seconds_after; i ++ )
		{
			// lts = Timestamp of the leap second
			int64_t	lts = leap_seconds_after[i] * 24*60*60 + 23*60*60 + 59*60 + 60 + i;
			if( lts > ts )	break;
			if( lts == ts )
				is_ls = true;
			n_leap ++;
		}
	}
	ts -= n_leap;
	#endif
	
	int64_t	days = ts / 24*60*60;
	int64_t seconds = ts % (24*60*60);
	*s = (is_ls ? 60 : seconds % 60);
	*m = (seconds/60 % 24);
	*h = seconds / (60*60);
	
	return days;
}

/**
 * \param days	Days since 1 Jan 2000
 */
int64_t get_years_since_y2k(int64_t days, bool *is_leap, int *doy)
{
	// Calculate Year
	bool	is_ly = false;
	// Year (400 yr blocks) - (400/4-3) leap years
	const int	days_per_400_yrs = 365*400 + (400/4-3);
	 int year = 400 * days / days_per_400_yrs;
	days = days % days_per_400_yrs;
	if( days < 366 )	// First year in 400 is a leap
		is_ly = true;
	else
	{
		// 100 yr blocks - 100/4-1 leap years
		const int	days_per_100_yrs = 365*100 + (100/4-1);
		year += 100 * days / days_per_100_yrs;
		days = days % days_per_100_yrs;
		if( days < 366 )	// First year in 100 isn't a leap
			is_ly = false;
		else
		{
			const int	days_per_4_yrs = 365*4 + 1;
			year += 4 * days / days_per_4_yrs;
			days = days % days_per_4_yrs;
			if( days < 366 )	// First year in 4 is a leap
				is_ly = true;
			else {
				year += days / 365;
				days = days % 365;
			}
		}
	}
	*doy = days;
	*is_leap = is_ly;
	
	return year;
}

void get_month_day(int doy, bool is_ly, int *mon, int *day)
{
	if( doy > 365+(is_ly?1:0) ) {
		*mon = 0;
		*day = 0;
	}
	
	bool mon_day_set = false;
	if( doy >= DAYS_BEFORE[2] && is_ly )
	{
		if( doy == DAYS_BEFORE[2] ) {
			*mon = 2;
			*day = 29;
			mon_day_set = true;
		}
		doy --;
	}
	for( int i = 1; i < 12+1 && !mon_day_set; i ++ )
	{
		if( doy < DAYS_BEFORE[i] )
		{
			*mon = i;
			*day = doy - DAYS_BEFORE[i];
			mon_day_set = true;
		}
	}
}

int expand_from_secs_since_y2k(int64_t ts, int *years_since, int *mon, int *day, int *h, int *m, int *s)
{
	int64_t	days = get_days_since_y2k(ts, h, m, s);
	
	bool	is_ly;
	 int	doy;
	*years_since = get_years_since_y2k(days, &is_ly, &doy);

	// Calculate month/day of month
	get_month_day(doy, is_ly, mon, day);
	return 0;
}


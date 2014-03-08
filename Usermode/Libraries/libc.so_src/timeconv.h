/*
 * Acess2 C Library
 * - By John Hodge (thePowersGang)
 *
 * timeconv.h
 * - Shared User/Kernel time conversion code
 */
#ifndef _LIBC_TIMECONV_H_
#define _LIBC_TIMECONV_H_

#include <stdint.h>
#include <stdbool.h>

extern int64_t	seconds_since_y2k(int years_since, int mon, int day, int h, int m, int s);

/**
 * \return Number of days since Y2K
 * 
 * Extracts the time of day from the timestamp, (possibly taking into account leap seconds)
 */
extern int64_t	get_days_since_y2k(int64_t ts, int *h, int *m, int *s);

/**
 * \param days	Number of days since Y2K
 * \return Number of years since Y2K
 * 
 * Converts a count of days since Y2K, and returns the number of years. Extracting the day of
 * year, and if that year is a leap year.
 */
extern int64_t	get_years_since_y2k(int64_t days, bool *is_leap, int *doy);


/**
 * Gets the month and day of month from a day of year and leap year status
 */
extern void	get_month_day(int doy, bool is_ly, int *mon, int *day);

#endif


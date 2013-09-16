/**
 * \file time.c
 * \author John Hodge (thePowersGang)
 */
#include <acess.h>
#include <udi.h>

// === EXPORTS ===
EXPORT(udi_timer_start);
EXPORT(udi_timer_start_repeating);
EXPORT(udi_timer_cancel);
EXPORT(udi_time_current);
EXPORT(udi_time_between);
EXPORT(udi_time_since);

// === CODE ===
void udi_timer_start(udi_timer_expired_call_t *callback, udi_cb_t *gcb, udi_time_t interval)
{
	UNIMPLEMENTED();
}
void udi_timer_start_repeating(udi_timer_tick_call_t *callback, udi_cb_t *gcb, udi_time_t interval)
{
	UNIMPLEMENTED();
}
void udi_timer_cancel(udi_cb_t *gcb)
{
	UNIMPLEMENTED();
}

udi_timestamp_t	udi_time_current(void)
{
	return now();
}
udi_time_t udi_time_between(udi_timestamp_t start_time, udi_timestamp_t end_time)
{
	udi_time_t	ret;
	tTime	delta = end_time - start_time;
	ret.seconds = delta / 1000;
	ret.nanoseconds = (delta % 1000) * 1000 * 1000;
	return ret;
}
udi_time_t udi_time_since(udi_timestamp_t start_time)
{
	return udi_time_between(start_time, udi_time_current());
}

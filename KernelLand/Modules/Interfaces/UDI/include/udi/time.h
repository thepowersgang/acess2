
#ifndef _UDI_TIME_H_
#define _UDI_TIME_H_

typedef struct {
	udi_ubit32_t	seconds;
	udi_ubit32_t	nanoseconds;
} udi_time_t;

typedef void udi_timer_expired_call_t(udi_cb_t *gcb);
typedef void udi_timer_tick_call_t(udi_cb_t *gcb, udi_ubit32_t missed);

extern void udi_timer_start(udi_timer_expired_call_t *callback, udi_cb_t *gcb, udi_time_t interval);
extern void udi_timer_start_repeating(udi_timer_tick_call_t *callback, udi_cb_t *gcb, udi_time_t interval);
extern void udi_timer_cancel(udi_cb_t *gcb);

extern udi_timestamp_t	udi_time_current(void);
extern udi_time_t	udi_time_between(udi_timestamp_t start_time, udi_timestamp_t end_time);
extern udi_time_t	udi_time_since(udi_timestamp_t start_time);

#endif


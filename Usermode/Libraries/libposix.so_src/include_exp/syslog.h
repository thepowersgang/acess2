/*
 * Acess2 POSIX Emulation
 * - By John Hodge (thePowersGang)
 *
 * syslog.h
 * - Centra Loggin
 */
#ifndef _LIBPOSIX__SYSLOG_H_
#define _LIBPOSIX__SYSLOG_H_

// openlog(logopt)
#define LOG_PID 	0x01
#define LOG_CONS	0x02
#define LOG_NDELAY	0x04
#define LOG_ODELAY	0x08
#define LOG_NOWAIT	0x10

// openlog(facility)
enum {
	LOG_KERN,
	LOG_USER,
	LOG_MAIL,
	LOG_NEWS,
	LOG_UUCP,
	LOG_DAEMON,
	LOG_AUTH,
	LOG_CRON,
	LOG_LPR,
	LOG_LOCAL0,
	LOG_LOCAL1,
	LOG_LOCAL2,
	LOG_LOCAL3,
	LOG_LOCAL4,
	LOG_LOCAL5,
	LOG_LOCAL6,
	LOG_LOCAL7
};

// setlogmask(maskpri)
#define LOG_MASK(pri)	pri

// syslog(priority)
enum {
	LOG_EMERG,
	LOG_ALERT,
	LOG_CRIT,
	LOG_ERR,
	LOG_WARNING,
	LOG_NOTICE,
	LOG_INFO,
	LOG_DEBUG
};

extern void	closelog(void);
extern void	openlog(const char *, int, int);
extern int	setlogmask(int);
extern void	syslog(int, const char *, ...);

#endif


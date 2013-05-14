/*
 * Acess2 POSIX Emulation
 * - By John Hodge (thePowersGang)
 *
 * syslog.h
 * - Central Logging
 */
#include <syslog.h>
#include <stdio.h>	// vsnprintf
#include <stdlib.h>	// free
#include <string.h>	// strdup
#include <stdarg.h>
#include <acess/sys.h>

// === GLOBALS ===
char	*gsSyslogPrefix = NULL;
 int	gSyslogOptions;
 int	gSyslogFacility;
 int	gSyslogMask = -1;

// === CODE ===
/*
 * Close global logging handle
 */
void closelog(void)
{
}

void openlog(const char *name, int option, int facility)
{
	if( gsSyslogPrefix )
		free(gsSyslogPrefix);
	gsSyslogPrefix = strdup(name);
	gSyslogOptions = option;
	gSyslogFacility = facility;
	
	if( option & LOG_NOWAIT )
	{
		// Open the logging handle!
	}
}

extern int setlogmask(int mask)
{
	 int	ret = gSyslogMask;
	gSyslogMask = mask;
	return ret;
}

extern void syslog(int priority, const char *str, ...)
{
	char	staticbuf[512];
	va_list	args;
	va_start(args, str);
	vsnprintf(staticbuf, sizeof(staticbuf), str, args);
	va_end(args);
	if( gSyslogOptions & (1 << priority) )
	{
		// TODO: Proper syslog
		_SysDebug("syslog(%i: %s) - %s", priority, gsSyslogPrefix, staticbuf);
	}
}


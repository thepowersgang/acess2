/*
 * Acess2 POSIX Emulation
 * - By John Hodge (thePowersGang)
 *
 * sys_resource.c
 * - (XSI) Resource Operations
 */
#include <sys/resource.h>
#include <acess/sys.h>

// === CODE ===
int getrlimit(int resource, struct rlimit *rlim)
{
	_SysDebug("TODO: getrlimit(%i)", resource);
	rlim->rlim_cur = 0;
	rlim->rlim_max = 0;
	return 0;
}

int setrlimit(int resource, const struct rlimit *rlim)
{
	_SysDebug("TODO: setrlimit(%i,{%i,%i})", resource, rlim->rlim_cur, rlim->rlim_max);
	return 0;
}


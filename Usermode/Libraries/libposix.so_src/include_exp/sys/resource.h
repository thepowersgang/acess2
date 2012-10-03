/*
 * Acess2 POSIX Emulation
 * - By John Hodge (thePowersGang)
 *
 * sys/resource.h
 * - (XSI) Resource Operations
 */
#ifndef _LIBPOSIX__SYS__RESOURCE_H_
#define _LIBPOSIX__SYS__RESOURCE_H_

#include <sys/time.h>	// struct timeval

// (get|set)priority(which)
enum
{
	PRIO_PROCESS,
	PRIO_PGRP,
	PRIO_USER
};

typedef unsigned int	rlim_t;
#define RLIM_INFINITY	-1
#define RLIM_SAVED_MAX	-2
#define RLIM_SAVED_CUR	-3

struct rlimit
{
	rlim_t	rlim_cur;
	rlim_t	rlim_max;
};

struct rusage
{
	struct timeval	ru_time;
	struct timeval	ru_stime;
};

extern int	getpriority(int, id_t);
extern int	getrlimit(int, struct rlimit *);
extern int	getrusage(int, struct rusage *);
extern int	setpriority(int, id_t, int);
extern int	setrlimit(int, const struct rlimit *);

#endif


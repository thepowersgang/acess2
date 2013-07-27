/*
 * AcessOS Basic C Library
 * signals.c
*/
#include <acess/sys.h>
#include <stdlib.h>
#include <signal.h>
#include "lib.h"
#include <errno.h>

// === CONSTANTS ===
#define NUM_SIGNALS	40

// === GLOBALS ===
struct sigaction	sighandlers[NUM_SIGNALS];

// === CODE ===
sighandler_t signal(int num, sighandler_t handler)
{
	sighandler_t	prev;
	if(num < 0 || num >= NUM_SIGNALS)	return NULL;
	prev = sighandlers[num].sa_handler;
	sighandlers[num].sa_handler = handler;
	sighandlers[num].sa_mask = 0;
	return prev;
}

int raise(int signal)
{
	if( signal < 0 || signal > NUM_SIGNALS )
		return 1;
	switch(signal)
	{
	case SIGABRT:
		abort();
		break;
	}
	return 0;
}

void abort(void)
{
	// raise(SIGABRT);
	_exit(-1);
}

// POSIX
int sigaction(int signum, const struct sigaction *act, struct sigaction *oldact)
{
	if( signum < 0 || signum >= NUM_SIGNALS ) {
		errno = EINVAL;
		return 1;
	}

	if( oldact )
		*oldact = sighandlers[signum];
	if( act )
		sighandlers[signum] = *act;	

	return 0;
}

int sigemptyset(sigset_t *set)
{
	*set = 0;
	return 0;
}
int sigfillset(sigset_t *set)
{
	*set = -1;
	return 0;
}

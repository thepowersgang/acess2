/*
 * AcessOS Basic C Library
 * signals.c
*/
#include <acess/sys.h>
#include <stdlib.h>
#include <signal.h>
#include "lib.h"

// === CONSTANTS ===
#define NUM_SIGNALS	32

// === GLOBALS ===
sighandler_t	sighandlers[NUM_SIGNALS];

// === CODE ===
sighandler_t signal(int num, sighandler_t handler)
{
	sighandler_t	prev;
	if(num < 0 || num >= NUM_SIGNALS)	return NULL;
	prev = sighandlers[num];
	sighandlers[num] = handler;
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

/*
 * AcessOS Basic C Library
 * signals.c
*/
#include <syscalls.h>
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

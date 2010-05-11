/*
 * Acess2 x86_64 Port
 * 
 * proc.h - Process/Thread management code
 */
#ifndef _PROC_H_
#define _PROC_H_

#include <arch.h>

typedef struct sMemoryState
{
	tPAddr	CR3;
}	tMemoryState;

typedef struct sTaskState
{
	Uint	RIP, RSP, RBP;
}	tTaskState;

#endif


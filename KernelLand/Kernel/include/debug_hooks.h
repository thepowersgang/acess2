/*
 * Acess2 Kernel
 * - By John Hodge (thePowersGang)
 *
 * debug_hooks.h
 * - Prototypes for methods used by keyboard/serial debugging
 */
#ifndef _DEBUG_HOOKS_H_
#define _DEBUG_HOOKS_H_

extern void	Heap_Dump(void);
extern void	Threads_Dump(void);
extern void	Threads_ToggleTrace(int TID);
extern void	Heap_Stats(void);

#endif

/*
 * Acess2 Kernel
 * - By John Hodge (thePowersGang)
 *
 * events.h
 * - Thread Events
 */
#ifndef _EVENTS_H_
#define _EVENTS_H_

#include <threads.h>

/**
 * \name Event Values
 * \{
 */
//! Fired when a VFS wait is ready [used in select(2)]
#define THREAD_EVENT_VFS	0x00000001
//! Fired when an IPC Message arrives
#define THREAD_EVENT_IPCMSG	0x00000002
//! Fired when a signal (e.g. SIGINT) is asserted
#define THREAD_EVENT_SIGNAL	0x00000004
//! Timer event fire
#define THREAD_EVENT_TIMER	0x00000008
//! General purpose event for short waits
//! e.g. waiting for an IRQ in a Read() call
#define THREAD_EVENT_SHORTWAIT	0x00000010
//! Fired when a child process quits
#define THREAD_EVENT_DEADCHILD	0x00000020

#define THREAD_EVENT_USER1	0x10000000
#define THREAD_EVENT_USER2	0x20000000
#define THREAD_EVENT_USER3	0x40000000
#define THREAD_EVENT_USER4	0x80000000
/**
 * \}
 */

// === FUNCTIONS ===
extern void	Threads_PostEvent(tThread *Thread, Uint32 EventMask);
extern void	Threads_ClearEvent(Uint32 EventMask);
extern Uint32	Threads_WaitEvents(Uint32 EventMask);

#endif


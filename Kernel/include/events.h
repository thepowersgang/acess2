/*
 * Acess2 Kernel
 * - By John Hodge (thePowersGang)
 *
 * events.h
 * - Thread Events
 */
#ifndef _EVENTS_H_
#define _EVENTS_H_

#define THREAD_EVENT_VFS	0x00000001
#define THREAD_EVENT_IPCMSG	0x00000002
#define THREAD_EVENT_SIGNAL	0x00000004

// === FUNCTIONS ===
extern void	Threads_PostEvent(tThread *Thread, Uint32 EventMask);
extern Uint32	Threads_WaitEvents(Uint32 EventMask);

#endif


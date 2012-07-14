/*
 * Acess2 Disk Tool
 */
#ifndef _RWLOCK_H
#define _RWLOCK_H

typedef char	tRWLock;

static inline int RWLock_AcquireRead(tRWLock *m) {
	if(*m)	Log_KernelPanic("---", "Double mutex lock");
	*m = 1;
	return 0;
}
static inline int RWLock_AcquireWrite(tRWLock *m) {
	if(*m)	Log_KernelPanic("---", "Double mutex lock");
	*m = 1;
	return 0;
}
static inline void RWLock_Release(tRWLock *m) { *m = 0; }

#endif


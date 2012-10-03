
#ifndef _MUTEX_H_
#define _MUTEX_H_

typedef struct {
	void	*LockerReturnAddr;
}	tMutex;

static inline int Mutex_Acquire(tMutex *m) {
	if(m->LockerReturnAddr)
		Log_KernelPanic("---", "Double mutex lock of %p by %p (was locked by %p)",
			m, __builtin_return_address(0), m->LockerReturnAddr);
	m->LockerReturnAddr = __builtin_return_address(0);;
	return 0;
}
static inline void Mutex_Release(tMutex *m) { m->LockerReturnAddr = 0; }

#endif


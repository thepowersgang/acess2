
int _errno;

#define SYSCALL0(name,num)	void name(void){}
#define SYSCALL1(name,num)	void name(void){}
#define SYSCALL2(name,num)	void name(void){}
#define SYSCALL3(name,num)	void name(void){}
#define SYSCALL4(name,num)	void name(void){}
#define SYSCALL5(name,num)	void name(void){}
#define SYSCALL6(name,num)	void name(void){}

#include "arch/syscalls.s.h"


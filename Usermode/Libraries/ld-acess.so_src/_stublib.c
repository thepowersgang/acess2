
#include <stdint.h>

int _errno;

#define SYSCALL0(name,num)	void name(void){}
#define SYSCALL1(name,num)	void name(void){}
#define SYSCALL2(name,num)	void name(void){}
#define SYSCALL3(name,num)	void name(void){}
#define SYSCALL4(name,num)	void name(void){}
#define SYSCALL5(name,num)	void name(void){}
#define SYSCALL6(name,num)	void name(void){}

#include "arch/syscalls.s.h"

// libgcc functions
#if 0
uint64_t __udivdi3(uint64_t Num, uint64_t Den){return 0;}
uint64_t __umoddi3(uint64_t Num, uint64_t Den){return 0;}

int32_t __divsi3(int32_t Num, int32_t Den){return 0;}
int32_t __modsi3(int32_t Num, int32_t Den){return 0;}
uint32_t __udivsi3(uint32_t Num, uint32_t Den){return 0;}
uint32_t __umodsi3(uint32_t Num, uint32_t Den){return 0;}
#endif

void	*_crt0_exit_handler;


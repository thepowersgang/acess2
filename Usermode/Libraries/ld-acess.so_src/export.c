/*
 * Acess2 Dynamic Linker
 */
#include "common.h"
#include <acess/sys.h>

extern uint64_t	__udivdi3(uint64_t Num, uint64_t Den);
extern uint64_t	__umoddi3(uint64_t Num, uint64_t Den);
extern int32_t	__divsi3(int32_t Num, int32_t Den);
extern int32_t	__modsi3(int32_t Num, int32_t Den);
extern uint32_t	__udivsi3(uint32_t Num, uint32_t Den);
extern uint32_t	__umodsi3(uint32_t Num, uint32_t Den);


#define _STR(x)	#x
#define STR(x)	_STR(x)
#define EXP(sym)	{&sym, STR(sym)}

// === CONSTANTS ===
const struct {
	void	*Value;
	char	*Name;
}	caLocalExports[] = {
	EXP(gLoadedLibraries),
	EXP(_exit),
	EXP(clone),
	EXP(kill),
	EXP(yield),
	EXP(sleep),
	EXP(_SysWaitEvent),
	EXP(waittid),
	EXP(gettid),
	EXP(getpid),
	EXP(getuid),
	EXP(getgid),

	EXP(setuid),
	EXP(setgid),

	EXP(SysSetName),
	//EXP(SysGetName),

	//EXP(SysSetPri),

	EXP(SysSendMessage),
	EXP(SysGetMessage),

	EXP(_SysSpawn),
	EXP(execve),
	EXP(SysLoadBin),
	EXP(SysUnloadBin),

	EXP(_SysSetFaultHandler),
	
	EXP(open),
	EXP(reopen),
	EXP(close),
	EXP(read),
	EXP(write),
	EXP(seek),
	EXP(tell),
	EXP(finfo),
	EXP(readdir),
	EXP(_SysGetACL),
	EXP(chdir),
	EXP(ioctl),
	EXP(_SysMount),
	EXP(_SysSelect),

	EXP(_SysOpenChild),
	
	EXP(_SysGetPhys),
	EXP(_SysAllocate),
	EXP(_SysDebug),

	EXP(__umoddi3),
	EXP(__udivdi3),
	EXP(__divsi3),
	EXP(__modsi3),
	EXP(__udivsi3),
	EXP(__umodsi3)
};

const int	ciNumLocalExports = sizeof(caLocalExports)/sizeof(caLocalExports[0]);

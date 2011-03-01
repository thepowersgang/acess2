/*
 * Acess2 Dynamic Linker
 */
#include "common.h"
#include <acess/sys.h>

#define _STR(x)	#x
#define STR(x)	_STR(x)
#define EXP(sym)	{(Uint)&sym, STR(sym)}

// === CONSTANTS ===
const struct {
	Uint	Value;
	char	*Name;
}	caLocalExports[] = {
	EXP(gLoadedLibraries),
	EXP(_exit),
	EXP(clone),
	EXP(kill),
	EXP(yield),
	EXP(sleep),
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

	//EXP(SysSpawn),
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
	EXP(select),

	EXP(_SysOpenChild),
	
	EXP(_SysGetPhys),
	EXP(_SysAllocate),
	EXP(_SysDebug)

};

const int	ciNumLocalExports = sizeof(caLocalExports)/sizeof(caLocalExports[0]);

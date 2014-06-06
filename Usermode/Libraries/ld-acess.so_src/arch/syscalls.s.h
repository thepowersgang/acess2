#include "../../../../KernelLand/Kernel/include/syscalls.h"

// --- Process Control ---
SYSCALL1(_exit, SYS_EXIT)

SYSCALL2(_SysClone, SYS_CLONE)
SYSCALL2(_SysKill, SYS_KILL)
//SYSCALL0(yield, SYS_YIELD)
//SYSCALL0(sleep, SYS_SLEEP)
SYSCALL1(_SysTimedSleep, SYS_TIMEDSLEEP)
SYSCALL1(_SysWaitEvent, SYS_WAITEVENT)
SYSCALL2(_SysWaitTID, SYS_WAITTID)

SYSCALL0(gettid, SYS_GETTID)
SYSCALL0(_SysGetPID, SYS_GETPID)
SYSCALL0(_SysGetUID, SYS_GETUID)
SYSCALL0(_SysGetGID, SYS_GETGID)

SYSCALL1(setuid, SYS_SETUID)
SYSCALL1(setgid, SYS_SETGID)

SYSCALL1(_SysSetName, SYS_SETNAME)
SYSCALL2(_SysGetName, SYS_GETNAME)
SYSCALL0(_SysTimestamp, SYS_GETTIME)

SYSCALL1(_SysSetPri, SYS_SETPRI)

SYSCALL3(_SysSendMessage, SYS_SENDMSG)
SYSCALL3(_SysGetMessage, SYS_GETMSG)

SYSCALL5(_SysSpawn, SYS_SPAWN)
SYSCALL3(_SysExecVE, SYS_EXECVE)
SYSCALL2(_SysLoadBin, SYS_LOADBIN)
SYSCALL1(_SysUnloadBin, SYS_UNLOADBIN)

SYSCALL1(_SysSetFaultHandler, SYS_SETFAULTHANDLER)

SYSCALL1(_SysLoadModule, SYS_LOADMOD)

SYSCALL6(_ZN4_sys5debugEPKcz, 0x100)
SYSCALL6(_SysDebug, 0x100)
SYSCALL2(_SysDebugHex, 0x101)

SYSCALL1(_SysGetPhys, SYS_GETPHYS)	// uint64_t _SysGetPhys(uint addr)
SYSCALL1(_SysAllocate, SYS_ALLOCATE)	// uint64_t _SysAllocate(uint addr)
SYSCALL3(_SysSetMemFlags, SYS_SETFLAGS)	// uint32_t SysSetMemFlags(uint addr, uint flags, uint mask)
// VFS System calls
SYSCALL2(_SysOpen, SYS_OPEN)	// char*, int
SYSCALL3(_SysOpenChild, SYS_OPENCHILD)	// int, char*, int
SYSCALL3(_SysReopen, SYS_REOPEN)	// int, char*, int
SYSCALL2(_SysCopyFD, SYS_COPYFD)	// int, int
SYSCALL3(_SysFDFlags, SYS_FDCTL)	// int, int, int
SYSCALL1(_SysClose, SYS_CLOSE)	// int
SYSCALL3(_SysRead, SYS_READ)	// int, uint, void*
SYSCALL3(_SysWrite, SYS_WRITE)	// int, uint, void*
SYSCALL4(_SysSeek, SYS_SEEK)	// int, uint64_t, int
SYSCALL1(_SysTell, SYS_TELL)	// int
SYSCALL3(_SysFInfo, SYS_FINFO)	// int, void*, int
SYSCALL2(_SysReadDir, SYS_READDIR)	// int, char*
SYSCALL2(_SysGetACL,SYS_GETACL)	// int, void*
SYSCALL1(_SysChdir, SYS_CHDIR)	// char*
SYSCALL3(_SysIOCtl, SYS_IOCTL)	// int, int, void*
SYSCALL4(_SysMount, SYS_MOUNT)	// char*, char*, char*, char*
SYSCALL6(_SysSelect, SYS_SELECT)	// int, fd_set*, fd_set*, fd_set*, tTime*, uint32_t

SYSCALL1(_SysMkDir, SYS_MKDIR)	// const char*
SYSCALL1(_SysUnlink, SYS_UNLINK)	// const char*


#include "../../../../KernelLand/Kernel/include/syscalls.h"

// --- Process Control ---
SYSCALL1(_exit, SYS_EXIT)

SYSCALL2(clone, SYS_CLONE)
SYSCALL2(kill, SYS_KILL)
//SYSCALL0(yield, SYS_YIELD)
//SYSCALL0(sleep, SYS_SLEEP)
SYSCALL1(_SysWaitEvent, SYS_WAITEVENT)
SYSCALL2(waittid, SYS_WAITTID)

SYSCALL0(gettid, SYS_GETTID)
SYSCALL0(getpid, SYS_GETPID)
SYSCALL0(getuid, SYS_GETUID)
SYSCALL0(getgid, SYS_GETGID)

SYSCALL1(setuid, SYS_SETUID)
SYSCALL1(setgid, SYS_SETGID)

SYSCALL1(SysSetName, SYS_SETNAME)
SYSCALL2(SysGetName, SYS_GETNAME)
SYSCALL0(_SysTimestamp, SYS_GETTIME)

SYSCALL1(SysSetPri, SYS_SETPRI)

SYSCALL3(SysSendMessage, SYS_SENDMSG)
SYSCALL3(SysGetMessage, SYS_GETMSG)

SYSCALL5(_SysSpawn, SYS_SPAWN)
SYSCALL3(execve, SYS_EXECVE)
SYSCALL2(SysLoadBin, SYS_LOADBIN)
SYSCALL1(SysUnloadBin, SYS_UNLOADBIN)

SYSCALL1(_SysSetFaultHandler, SYS_SETFAULTHANDLER)

SYSCALL6(_SysDebug, 0x100)
SYSCALL1(_SysGetPhys, SYS_GETPHYS)	// uint64_t _SysGetPhys(uint addr)
SYSCALL1(_SysAllocate, SYS_ALLOCATE)	// uint64_t _SysAllocate(uint addr)
SYSCALL3(SysSetMemFlags, SYS_SETFLAGS)	// uint32_t SysSetMemFlags(uint addr, uint flags, uint mask)
// VFS System calls
SYSCALL2(_SysOpen, SYS_OPEN)	// char*, int
SYSCALL3(_SysOpenChild, SYS_OPENCHILD)	// int, char*, int
SYSCALL3(_SysReopen, SYS_REOPEN)	// int, char*, int
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
SYSCALL1(_SysUnlink, SYS_UNLINK)	// const char*


/*
 * AcessOS Microkernel Version
 * syscalls.h
 */
#ifndef _SYSCALLS_H
#define _SYSCALLS_H

enum eSyscalls {
	SYS_EXIT,	// 0 - Kill Current Thread
	SYS_CLONE,	// 1 - Create a new thread
	SYS_KILL,	// 2 - Send a signal
	SYS_SIGNAL,	// 3 - Set a signal handler
	SYS_YIELD,	// 4 - Yield remainder of timestamp
	SYS_SLEEP,	// 5 - Sleep until messaged or signaled
	SYS_WAIT,	// 6 - Wait for a time or a message
	SYS_WAITTID,	// 7 - Wait for a thread to terminate
	
	SYS_SETNAME,	// 8 - Set thread's name
	SYS_GETNAME,	// 9 - Get a thread's name
	SYS_GETTID,	// 10 - Get current thread ID
	SYS_GETPID,	// 11 - Get current thread group ID
	
	SYS_SETPRI,		// 12 - Set Priority
	
	SYS_SENDMSG,	// 13 - Send an IPC message
	SYS_GETMSG,		// 14 - Recieve an IPC message
	
	SYS_GETTIME,	// 15 - Get the current timestamp

	SYS_SPAWN,		// 16 - Fork and Execve
	SYS_EXECVE,		// 17 - Replace the current process image with another
	SYS_LOADBIN,	// 18 - Load a binary image into memory
	SYS_UNLOADBIN,	// 19 - Unload a loaded binary image
	
	// Address Space & Permissions (001x xxxx)
	SYS_GETPHYS=32,	// 32 - Gets the physical address of a page
	SYS_MAP,		// 33 - Map a physical page
	SYS_ALLOCATE,	// 34 - Allocate a page
	SYS_UNMAP,		// 35 - Unmap a page
	SYS_PREALLOC,	// 36 - Preallocates a page
	SYS_SETFLAGS,	// 37 - Sets a page's flags
	SYS_SHAREWITH,	// 38 - Share a page with another thread
	// Permissions
	SYS_GETUID,	// 39 - Get current User ID
	SYS_GETGID,	// 40 - Get current Group ID
	SYS_SETUID,	// 41 - Set the current User ID
	SYS_SETGID,	// 42 - Set the current Group ID
	
	// VFS (010x xxxx)
	SYS_OPEN = 64,	// 64 - Open a file
	SYS_REOPEN,		// 65 - Close a file and reuse its handle
	SYS_CLOSE,		// 66 - Close and open file
	SYS_READ,		// 67 - Read from a file
	SYS_WRITE,		// 68 - Write to a file
	SYS_IOCTL,		// 69 - Call a file's IOCtl method
	SYS_READDIR,	// 70 - Read from a directory
	SYS_MKDIR,		// 71 - Make new directory
	SYS_SYMLINK,	// 72 - Create a symbolic link
	SYS_GETACL,		// 73 - Get an ACL
	SYS_SETACL,		// 74 - Set an ACL
	SYS_FINFO,		// 75 - Get a file's information
	
	NUM_SYSCALLS,
	
	SYS_DEBUG = 0x100		// Print a debug string
};

static const char *cSYSCALL_NAMES[] = {
	"SYS_EXIT", "SYS_CLONE", "SYS_KILL", "SYS_SIGNAL", "SYS_YIELD", "SYS_SLEEP", "SYS_WAIT",
	"SYS_WAITTID",
	"SYS_SETNAME", "SYS_GETNAME", "SYS_GETTID", "SYS_GETPID", "SYS_SETPRI",
	"SYS_SENDMSG", "SYS_GETMSG",
	"SYS_GETTIME",
	"SYS_SPAWN", "SYS_EXECVE", "SYS_LOADBIN",
	"", "", "", "", "", "", "", "", "", "", "", "",
	
	"SYS_GETPHYS", "SYS_MAP", "SYS_ALLOCATE", "SYS_UNMAP",
	"SYS_PREALLOC", "SYS_SETFLAGS", "SYS_SHAREWITH",
	"SYS_GETUID", "SYS_GETGID", "SYS_SETUID", "SYS_SETGID",
	
	"", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
	
	"SYS_OPEN", "SYS_REOPEN", "SYS_CLOSE",
	"SYS_READ", "SYS_WRITE", "SYS_IOCTL",
	"SYS_READDIR", "SYS_MKDIR", "SYS_SYMLINK",
	"SYS_GETACL", "SYS_SETACL", "SYS_FINFO"
	};

#endif

/*
 * AcessOS Microkernel Version
 * syscalls.h
 */
#ifndef _SYSCALLS_H
#define _SYSCALLS_H

enum eSyscalls {
	SYS_EXIT,	// 0 - Kill this thread
	SYS_CLONE,	// 1 - Create a new thread
	SYS_KILL,	// 2 - Send a signal
	SYS_SETFAULTHANDLER,	// 3 - Set signal Handler
	SYS_YIELD,	// 4 - Yield remainder of timestamp
	SYS_SLEEP,	// 5 - Sleep until messaged or signaled
	SYS_WAIT,	// 6 - Wait for a time or a message
	SYS_WAITTID,	// 7 - Wait for a thread to do something
	SYS_SETNAME,	// 8 - Set's the name of the current thread
	SYS_GETNAME,	// 9 - Get's the name of a thread
	SYS_GETTID,	// 10 - Get current thread ID
	SYS_GETPID,	// 11 - Get current thread group ID
	SYS_SETPRI,	// 12 - Set process priority
	SYS_SENDMSG,	// 13 - Send an IPC message
	SYS_GETMSG,	// 14 - Recieve an IPC message
	SYS_GETTIME,	// 15 - Get the current timestamp
	SYS_SPAWN,	// 16 - Spawn a new process
	SYS_EXECVE,	// 17 - Replace the current process
	SYS_LOADBIN,	// 18 - Load a binary into the current address space
	SYS_UNLOADBIN,	// 19 - Unload a loaded binary
	SYS_LOADMOD,	// 20 - Load a module into the kernel

	SYS_GETPHYS = 32,	// 32 - Get the physical address of a page
	SYS_MAP,	// 33 - 
	SYS_ALLOCATE,	// 34 - Allocate a page
	SYS_UNMAP,	// 35 - Unmap a page
	SYS_PREALLOC,	// 36 - Preallocate a page
	SYS_SETFLAGS,	// 37 - Set a page's flags
	SYS_SHAREWITH,	// 38 - Share a page with another thread
	SYS_GETUID,	// 39 - Get current User ID
	SYS_GETGID,	// 40 - Get current Group ID
	SYS_SETUID,	// 41 - Set current user ID
	SYS_SETGID,	// 42 - Set current Group ID

	SYS_OPEN = 64,	// 64 - Open a file
	SYS_REOPEN,	// 65 - Close a file and reuse its handle
	SYS_CLOSE,	// 66 - Close a file
	SYS_READ,	// 67 - Read from an open file
	SYS_WRITE,	// 68 - Write to an open file
	SYS_IOCTL,	// 69 - Perform an IOCtl Call
	SYS_READDIR,	// 70 - Read from an open directory
	SYS_OPENCHILD,	// 71 - Open a child entry in a directory
	SYS_MKDIR,	// 72 - Create a new directory
	SYS_SYMLINK,	// 73 - Create a symbolic link
	SYS_GETACL,	// 74 - Get an ACL Value
	SYS_SETACL,	// 75 - Set an ACL Value
	SYS_FINFO,	// 76 - Get file information
	SYS_SEEK,	// 77 - Seek to a new position in the file
	SYS_TELL,	// 78 - Return the current file position
	SYS_CHDIR,	// 79 - Change current directory
	SYS_GETCWD,	// 80 - Get current directory
	SYS_MOUNT,	// 81 - Mount a filesystem
	NUM_SYSCALLS,
	SYS_DEBUG = 0x100	// 0x100 - Print a debug string
};

static const char *cSYSCALL_NAMES[] = {
	"SYS_EXIT","SYS_CLONE","SYS_KILL","SYS_SETFAULTHANDLER","SYS_YIELD","SYS_SLEEP",
	"SYS_WAIT","SYS_WAITTID","SYS_SETNAME","SYS_GETNAME","SYS_GETTID","SYS_GETPID",
	"SYS_SETPRI","SYS_SENDMSG","SYS_GETMSG","SYS_GETTIME","SYS_SPAWN","SYS_EXECVE",
	"SYS_LOADBIN","SYS_UNLOADBIN","SYS_LOADMOD","","","",
	"","","","","","",
	"","","SYS_GETPHYS","SYS_MAP","SYS_ALLOCATE","SYS_UNMAP",
	"SYS_PREALLOC","SYS_SETFLAGS","SYS_SHAREWITH","SYS_GETUID","SYS_GETGID","SYS_SETUID",
	"SYS_SETGID","","","","","",
	"","","","","","",
	"","","","","","",
	"","","","","SYS_OPEN","SYS_REOPEN",
	"SYS_CLOSE","SYS_READ","SYS_WRITE","SYS_IOCTL","SYS_READDIR","SYS_OPENCHILD",
	"SYS_MKDIR","SYS_SYMLINK","SYS_GETACL","SYS_SETACL","SYS_FINFO","SYS_SEEK",
	"SYS_TELL","SYS_CHDIR","SYS_GETCWD","SYS_MOUNT",""
};
#endif

/*
 * Acess2
 * syscalls.h
 * - System Call List
 *
 * NOTE: Generated from Kernel/syscalls.lst
 */
#ifndef _SYSCALLS_H
#define _SYSCALLS_H

enum eSyscalls {
	SYS_EXIT,	// Kill this thread
	SYS_CLONE,	// Create a new thread
	SYS_KILL,	// Send a signal
	SYS_SETFAULTHANDLER,	// Set signal Handler
	SYS_YIELD,	// Yield remainder of timestamp
	SYS_SLEEP,	// Sleep until messaged or signaled
	SYS_WAIT,	// Wait for a time or a message
	SYS_WAITTID,	// Wait for a thread to do something
	SYS_SETNAME,	// Set's the name of the current thread
	SYS_GETNAME,	// Get's the name of a thread
	SYS_GETTID,	// Get current thread ID
	SYS_GETPID,	// Get current thread group ID
	SYS_SETPRI,	// Set process priority
	SYS_SENDMSG,	// Send an IPC message
	SYS_GETMSG,	// Recieve an IPC message
	SYS_GETTIME,	// Get the current timestamp
	SYS_SPAWN,	// Spawn a new process
	SYS_EXECVE,	// Replace the current process
	SYS_LOADBIN,	// Load a binary into the current address space
	SYS_UNLOADBIN,	// Unload a loaded binary
	SYS_LOADMOD,	// Load a module into the kernel

	SYS_GETPHYS = 32,	// Get the physical address of a page
	SYS_MAP,	// Map a physical address
	SYS_ALLOCATE,	// Allocate a page
	SYS_UNMAP,	// Unmap a page
	SYS_PREALLOC,	// Preallocate a page
	SYS_SETFLAGS,	// Set a page's flags
	SYS_SHAREWITH,	// Share a page with another thread
	SYS_GETUID,	// Get current User ID
	SYS_GETGID,	// Get current Group ID
	SYS_SETUID,	// Set current user ID
	SYS_SETGID,	// Set current Group ID

	SYS_OPEN = 64,	// Open a file
	SYS_REOPEN,	// Close a file and reuse its handle
	SYS_CLOSE,	// Close a file
	SYS_READ,	// Read from an open file
	SYS_WRITE,	// Write to an open file
	SYS_IOCTL,	// Perform an IOCtl Call
	SYS_SEEK,	// Seek to a new position in the file
	SYS_READDIR,	// Read from an open directory
	SYS_OPENCHILD,	// Open a child entry in a directory
	SYS_GETACL,	// Get an ACL Value
	SYS_SETACL,	// Set an ACL Value
	SYS_FINFO,	// Get file information
	SYS_MKDIR,	// Create a new directory
	SYS_LINK,	// Create a new link to a file
	SYS_SYMLINK,	// Create a symbolic link
	SYS_UNLINK,	// Delete a file
	SYS_TELL,	// Return the current file position
	SYS_CHDIR,	// Change current directory
	SYS_GETCWD,	// Get current directory
	SYS_MOUNT,	// Mount a filesystem
	SYS_SELECT,	// Wait for file handles

	NUM_SYSCALLS,
	SYS_DEBUG = 0x100
};

static const char *cSYSCALL_NAMES[] = {
	"SYS_EXIT",
	"SYS_CLONE",
	"SYS_KILL",
	"SYS_SETFAULTHANDLER",
	"SYS_YIELD",
	"SYS_SLEEP",
	"SYS_WAIT",
	"SYS_WAITTID",
	"SYS_SETNAME",
	"SYS_GETNAME",
	"SYS_GETTID",
	"SYS_GETPID",
	"SYS_SETPRI",
	"SYS_SENDMSG",
	"SYS_GETMSG",
	"SYS_GETTIME",
	"SYS_SPAWN",
	"SYS_EXECVE",
	"SYS_LOADBIN",
	"SYS_UNLOADBIN",
	"SYS_LOADMOD",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"SYS_GETPHYS",
	"SYS_MAP",
	"SYS_ALLOCATE",
	"SYS_UNMAP",
	"SYS_PREALLOC",
	"SYS_SETFLAGS",
	"SYS_SHAREWITH",
	"SYS_GETUID",
	"SYS_GETGID",
	"SYS_SETUID",
	"SYS_SETGID",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"SYS_OPEN",
	"SYS_REOPEN",
	"SYS_CLOSE",
	"SYS_READ",
	"SYS_WRITE",
	"SYS_IOCTL",
	"SYS_SEEK",
	"SYS_READDIR",
	"SYS_OPENCHILD",
	"SYS_GETACL",
	"SYS_SETACL",
	"SYS_FINFO",
	"SYS_MKDIR",
	"SYS_LINK",
	"SYS_SYMLINK",
	"SYS_UNLINK",
	"SYS_TELL",
	"SYS_CHDIR",
	"SYS_GETCWD",
	"SYS_MOUNT",
	"SYS_SELECT",

	""
};

#endif

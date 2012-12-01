/*
 * Acess2
 * syscalls.h
 * - System Call List
 *
 * NOTE: Generated from Kernel/syscalls.lst
 */
#ifndef _SYSCALLS_H
#define _SYSCALLS_H

#define SYS_EXIT	0	// Kill this thread
#define SYS_CLONE	1	// Create a new thread
#define SYS_KILL	2	// Send a signal
#define SYS_SETFAULTHANDLER	3	// Set signal Handler
#define SYS_YIELD	4	// Yield remainder of timestamp
#define SYS_SLEEP	5	// Sleep until messaged or signaled
#define SYS_WAITEVENT	6	// Wait for an event
#define SYS_WAITTID	7	// Wait for a thread to do something
#define SYS_SETNAME	8	// Sets the name of the current thread
#define SYS_GETNAME	9	// Gets the name of a thread
#define SYS_GETTID	10	// Get current thread ID
#define SYS_GETPID	11	// Get current thread group ID
#define SYS_SETPRI	12	// Set process priority
#define SYS_SENDMSG	13	// Send an IPC message
#define SYS_GETMSG	14	// Recieve an IPC message
#define SYS_GETTIME	15	// Get the current timestamp
#define SYS_SPAWN	16	// Spawn a new process
#define SYS_EXECVE	17	// Replace the current process
#define SYS_LOADBIN	18	// Load a binary into the current address space
#define SYS_UNLOADBIN	19	// Unload a loaded binary
#define SYS_LOADMOD	20	// Load a module into the kernel
#define SYS_GETPHYS	32	// Get the physical address of a page
#define SYS_MAP	33	// Map a physical address
#define SYS_ALLOCATE	34	// Allocate a page
#define SYS_UNMAP	35	// Unmap a page
#define SYS_PREALLOC	36	// Preallocate a page
#define SYS_SETFLAGS	37	// Set a page's flags
#define SYS_SHAREWITH	38	// Share a page with another thread
#define SYS_GETUID	39	// Get current User ID
#define SYS_GETGID	40	// Get current Group ID
#define SYS_SETUID	41	// Set current user ID
#define SYS_SETGID	42	// Set current Group ID
#define SYS_OPEN	64	// Open a file
#define SYS_REOPEN	65	// Close a file and reuse its handle
#define SYS_OPENCHILD	66	// Open a child entry in a directory
#define SYS_OPENPIPE	67	// Open a FIFO pipe pair
#define SYS_CLOSE	68	// Close a file
#define SYS_COPYFD	69	// Create a copy of a file handle
#define SYS_FDCTL	70	// Modify properties of a file descriptor
#define SYS_READ	71	// Read from an open file
#define SYS_WRITE	72	// Write to an open file
#define SYS_IOCTL	73	// Perform an IOCtl Call
#define SYS_SEEK	74	// Seek to a new position in the file
#define SYS_READDIR	75	// Read from an open directory
#define SYS_GETACL	76	// Get an ACL Value
#define SYS_SETACL	77	// Set an ACL Value
#define SYS_FINFO	78	// Get file information
#define SYS_MKDIR	79	// Create a new directory
#define SYS_LINK	80	// Create a new link to a file
#define SYS_SYMLINK	81	// Create a symbolic link
#define SYS_UNLINK	82	// Delete a file
#define SYS_TELL	83	// Return the current file position
#define SYS_CHDIR	84	// Change current directory
#define SYS_GETCWD	85	// Get current directory
#define SYS_MOUNT	86	// Mount a filesystem
#define SYS_SELECT	87	// Wait for file handles

#define NUM_SYSCALLS	88
#define SYS_DEBUG	0x100

#ifndef __ASSEMBLER__
static const char *cSYSCALL_NAMES[] = {
	"SYS_EXIT",
	"SYS_CLONE",
	"SYS_KILL",
	"SYS_SETFAULTHANDLER",
	"SYS_YIELD",
	"SYS_SLEEP",
	"SYS_WAITEVENT",
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
	"SYS_OPENCHILD",
	"SYS_OPENPIPE",
	"SYS_CLOSE",
	"SYS_COPYFD",
	"SYS_FDCTL",
	"SYS_READ",
	"SYS_WRITE",
	"SYS_IOCTL",
	"SYS_SEEK",
	"SYS_READDIR",
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

#endif

/*
 * Acess2 Kernel
 * - By John Hodge (thePowersGang)
 *
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
#define SYS_TIMEDSLEEP	6	// Sleep until a specified time has elapsed
#define SYS_WAITEVENT	7	// Wait for an event
#define SYS_WAITTID	8	// Wait for a thread to do something
#define SYS_SETSIGNALHANDLER	9	// Set the POSIX signal handler
#define SYS_SETSIGNALMASK	10	// Sets the mask of disabled POSIX signals
#define SYS_SETNAME	11	// Sets the name of the current thread
#define SYS_GETNAME	12	// Gets the name of a thread
#define SYS_GETTID	13	// Get current thread ID
#define SYS_GETPID	14	// Get current thread group ID
#define SYS_SETPRI	15	// Set process priority
#define SYS_SENDMSG	16	// Send an IPC message
#define SYS_GETMSG	17	// Recieve an IPC message
#define SYS_GETTIME	18	// Get the current timestamp
#define SYS_SPAWN	19	// Spawn a new process
#define SYS_EXECVE	20	// Replace the current process
#define SYS_LOADBIN	21	// Load a binary into the current address space
#define SYS_UNLOADBIN	22	// Unload a loaded binary
#define SYS_LOADMOD	23	// Load a module into the kernel
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
#define SYS_FDCTL	70	// Modify flags of a file descriptor
#define SYS_READ	71	// Read from an open file
#define SYS_READAT	72	// Read from an open file (with offset)
#define SYS_WRITE	73	// Write to an open file
#define SYS_WRITEAT	74	// Write to an open file (with offset)
#define SYS_IOCTL	75	// Perform an IOCtl Call
#define SYS_SEEK	76	// Seek to a new position in the file
#define SYS_READDIR	77	// Read from an open directory
#define SYS_GETACL	78	// Get an ACL Value
#define SYS_SETACL	79	// Set an ACL Value
#define SYS_FINFO	80	// Get file information
#define SYS_MKDIR	81	// Create a new directory
#define SYS_LINK	82	// Create a new link to a file
#define SYS_SYMLINK	83	// Create a symbolic link
#define SYS_UNLINK	84	// Delete a file
#define SYS_TELL	85	// Return the current file position
#define SYS_CHDIR	86	// Change current directory
#define SYS_GETCWD	87	// Get current directory
#define SYS_MOUNT	88	// Mount a filesystem
#define SYS_SELECT	89	// Wait for file handles
#define SYS_MARSHALFD	90	// Create a reference to a FD suitable for handing to another process
#define SYS_UNMARSHALFD	91	// Accept a marshaled FD

#define NUM_SYSCALLS	92
#define SYS_DEBUG	0x100
#define SYS_DEBUGHEX	0x101

#if !defined(__ASSEMBLER__) && !defined(NO_SYSCALL_STRS)
static const char *cSYSCALL_NAMES[] = {
	"SYS_EXIT",
	"SYS_CLONE",
	"SYS_KILL",
	"SYS_SETFAULTHANDLER",
	"SYS_YIELD",
	"SYS_SLEEP",
	"SYS_TIMEDSLEEP",
	"SYS_WAITEVENT",
	"SYS_WAITTID",
	"SYS_SETSIGNALHANDLER",
	"SYS_SETSIGNALMASK",
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
	"SYS_READAT",
	"SYS_WRITE",
	"SYS_WRITEAT",
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
	"SYS_MARSHALFD",
	"SYS_UNMARSHALFD",

	""
};
#endif

#endif

; Acess2
; System Calls List
; 

%define SYS_EXIT	0	; Kill this thread
%define SYS_CLONE	1	; Create a new thread
%define SYS_KILL	2	; Send a signal
%define SYS_SIGNAL	3	; Set signal Handler
%define SYS_YIELD	4	; Yield remainder of timestamp
%define SYS_SLEEP	5	; Sleep until messaged or signaled
%define SYS_WAIT	6	; Wait for a time or a message
%define SYS_WAITTID	7	; Wait for a thread to do something
%define SYS_SETNAME	8	; Set's the name of the current thread
%define SYS_GETNAME	9	; Get's the name of a thread
%define SYS_GETTID	10	; Get current thread ID
%define SYS_GETPID	11	; Get current thread group ID
%define SYS_SETPRI	12	; Set process priority
%define SYS_SENDMSG	13	; Send an IPC message
%define SYS_GETMSG	14	; Recieve an IPC message
%define SYS_GETTIME	15	; Get the current timestamp
%define SYS_SPAWN	16	; Spawn a new process
%define SYS_EXECVE	17	; Replace the current process
%define SYS_LOADBIN	18	; Load a binary into the current address space
%define SYS_UNLOADBIN	19	; Unload a loaded binary
%define SYS_LOADMOD	20	; Load a module into the kernel

%define SYS_GETPHYS	32	; Get the physical address of a page
%define SYS_MAP	33	; 	Map a physical address
%define SYS_ALLOCATE	34	; Allocate a page
%define SYS_UNMAP	35	; Unmap a page
%define SYS_PREALLOC	36	; Preallocate a page
%define SYS_SETFLAGS	37	; Set a page's flags
%define SYS_SHAREWITH	38	; Share a page with another thread
%define SYS_GETUID	39	; Get current User ID
%define SYS_GETGID	40	; Get current Group ID
%define SYS_SETUID	41	; Set current user ID
%define SYS_SETGID	42	; Set current Group ID

%define SYS_OPEN	64	; Open a file
%define SYS_REOPEN	65	; Close a file and reuse its handle
%define SYS_CLOSE	66	; Close a file
%define SYS_READ	67	; Read from an open file
%define SYS_WRITE	68	; Write to an open file
%define SYS_IOCTL	69	; Perform an IOCtl Call
%define SYS_READDIR	70	; Read from an open directory
%define SYS_MKDIR	71	; Create a new directory
%define SYS_SYMLINK	72	; Create a symbolic link
%define SYS_GETACL	73	; Get an ACL Value
%define SYS_SETACL	74	; Set an ACL Value
%define SYS_FINFO	75	; Get file information
%define SYS_SEEK	76	; Seek to a new position in the file
%define SYS_TELL	77	; Return the current file position
%define SYS_CHDIR	78	; Change current directory
%define SYS_GETCWD	79	; Get current directory
%define SYS_MOUNT	80	; Mount a filesystem

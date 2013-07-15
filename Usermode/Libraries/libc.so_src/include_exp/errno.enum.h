
enum {
	EOK,
	ENOSYS,	// Invalid Instruction
	EINVAL,	// Invalid Paramater
	EBADF,	// Bad FD
	ENOMEM,	// No free memory
	EACCES,	// Not permitted
	EBUSY,	// Resource is busy
	ERANGE,	// Value out of range
	ENOTFOUND,	// Item not found
	EREADONLY,	// Read only (duplicate with EROFS?)
	ENOTIMPL,	// Not implemented
	ENOENT,	// No entry?
	EEXIST,	// Already exists
	ENFILE,	// Too many open files
	ENOTDIR,	// Not a directory
	EISDIR,	// Is a directory
	EIO,	// IO Error
	EINTR,	// Operation interrupted (signal)
	EWOULDBLOCK,	// Operation would have blocked
	ENODEV,	// ???
	EADDRNOTAVAIL,	// ?
	EINPROGRESS,	// ?
	EROFS,
	EPERM,	// Permissions error

	EAGAIN,	// Try again
	
	EALREADY,	// Operation was a NOP
	EINTERNAL,	// Internal Error
	
	NUM_ERRNO
};


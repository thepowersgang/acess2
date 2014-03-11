
enum libc_eErrorNumbers {
	EOK,
	ENOSYS,	// Invalid Instruction
	EINVAL,	// Invalid Paramater
	EBADF,	// Bad FD
	ENOMEM,	// No free memory
	EACCES,	// Not permitted
	EBUSY,	// Resource is busy
	ERANGE,	// Value out of range
	ENOTFOUND,	// Item not found
	EROFS,	// Read only
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
	EPERM,	// Permissions error
	ENOTTY,	// not a tty

	EAGAIN,	// Try again
	EALREADY,	// Operation was a NOP
	
	EFBIG,	// File too large

	// psockets
	EAFNOSUPPORT,	
	
	EINTERNAL	// Internal Error
};
	
#define NUM_ERRNO	(EINTERNAL+1)


/*
 * Acess2
 * errno.h
 */
#ifndef _ERRNO_H
#define _ERRNO_H

enum eErrorNums
{
	EOK,
	
	ENOSYS,	// Invalid Instruction
	EINVAL,	// Invalid Paramater
	ENOMEM,	// No free memory
	EACCES,	// Not permitted
	EBUSY,	// Resource is busy
	ENOTFOUND,	// Item not found
	EREADONLY,	// Read only
	ENOTIMPL,	// Not implemented
	ENOENT,	// No entry?
	EEXIST,	// Already exists
	ENFILE,	// Too many open files
	ENOTDIR,	// Not a directory
	EIO,	// IO Error
	
	EALREADY,	// Operation was a NOP
	EINTERNAL,	// Internal Error
	
	NUM_ERRS
};

#endif

/**
 */
#ifndef _NATIVE_SYSCALLS_H_
#define _NATIVE_SYSCALLS_H_

enum eSyscalls {
	SYS_NULL,
	SYS_OPEN
};

enum eArgumentTypes {
	ARG_TYPE_VOID,
	ARG_TYPE_INT32,
	ARG_TYPE_INT64,
	ARG_TYPE_STRING,
	ARG_TYPE_DATA
};

#define ARG_DIR_TOSRV	0x10
#define	ARG_DIR_TOCLI	0x20
#define ARG_DIR_BOTH	0x30

#endif

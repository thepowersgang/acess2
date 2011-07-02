/**
 */
#ifndef _NATIVE_SYSCALLS_H_
#define _NATIVE_SYSCALLS_H_

#define	SERVER_PORT	0xACE

/*
 * Request format
 * 
 * tRequestHeader	header
 * tRequestValue	params[header.NParams]
 * tRequestValue	retvals[header.NReturn]
 * uint8_t	paramData[SUM(params[].Lengh)];
 */

typedef struct sRequestValue {
	/// \see eArgumentTypes
	uint16_t	Type;
	uint16_t	Flags;
	uint16_t	Length;
}	tRequestValue;

typedef struct sRequestHeader {
	uint16_t	ClientID;
	uint16_t	CallID;	//!< \see eSyscalls
	uint16_t	NParams;
	
	tRequestValue	Params[];
}	tRequestHeader;

enum eSyscalls {
	SYS_NULL,
	
	SYS_EXIT,
	
	SYS_OPEN,
	SYS_CLOSE,
	SYS_READ,
	SYS_WRITE,
	SYS_SEEK,
	SYS_TELL,
	SYS_IOCTL,
	SYS_FINFO,
	SYS_READDIR,
	SYS_OPENCHILD,
	SYS_GETACL,
	SYS_MOUNT,
	SYS_REOPEN,
	SYS_CHDIR,
	
	SYS_WAITTID,
	SYS_SETUID,
	SYS_SETGID,
	
	// IPC
	SYS_SLEEP,
	SYS_FORK,
	SYS_SENDMSG,
	SYS_GETMSG,
	SYS_SELECT,
	
	N_SYSCALLS
};

#ifndef DONT_INCLUDE_SYSCALL_NAMES
static const char * casSYSCALL_NAMES[] = {
	"SYS_NULL",
	
	"SYS_EXIT",
	
	"SYS_OPEN",
	"SYS_CLOSE",
	"SYS_READ",
	"SYS_WRITE",
	"SYS_SEEK",
	"SYS_TELL",
	"SYS_IOCTL",
	"SYS_FINFO",
	"SYS_READDIR",
	"SYS_OPENCHILD",
	"SYS_GETACL",
	"SYS_MOUNT",
	"SYS_REOPEN",
	
	// IPC
	"SYS_SLEEP"
};
#endif

enum eArgumentTypes {
	ARG_TYPE_VOID,
	ARG_TYPE_INT32,
	ARG_TYPE_INT64,
	ARG_TYPE_STRING,
	ARG_TYPE_DATA
};
enum eArgumentFlags {
	ARG_FLAG_RETURN = 0x40,	// Pass back in the return message
	ARG_FLAG_ZEROED = 0x80	// Not present in the message, just fill with zero
};

#endif

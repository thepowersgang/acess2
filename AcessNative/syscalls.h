/**
 */
#ifndef _NATIVE_SYSCALLS_H_
#define _NATIVE_SYSCALLS_H_

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
	uint16_t	Length;
}	tRequestValue;

typedef struct sRequestHeader {
	uint16_t	ClientID;
	uint16_t	CallID;	//!< \see eSyscalls
	uint16_t	NParams;
	uint16_t	NReturn;
	
	tRequestValue	Params[];
}	tRequestHeader;

enum eSyscalls {
	SYS_NULL,
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
	N_SYSCALLS
};

enum eArgumentTypes {
	ARG_TYPE_VOID,
	ARG_TYPE_INT32,
	ARG_TYPE_INT64,
	ARG_TYPE_STRING,
	ARG_TYPE_DATA
};

#endif

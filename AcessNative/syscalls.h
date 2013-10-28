/**
 */
#ifndef _NATIVE_SYSCALLS_H_
#define _NATIVE_SYSCALLS_H_

#define	SERVER_PORT	0xACE

#define SYSCALL_TRACE	1

#if SYSCALL_TRACE
#define SYSTRACE(str, x...)	do{ if(gbSyscallDebugEnabled)Debug(str, x); }while(0)
#else
#define SYSTRACE(...)	do{}while(0)
#endif

/*
 * Request format
 * 
 * tRequestHeader	header
 * tRequestValue	params[header.NParams]
 * tRequestValue	retvals[header.NReturn]
 * uint8_t	paramData[SUM(params[].Lengh)];
 */

typedef struct {
	uint32_t	pid;
	uint32_t	key;
} tRequestAuthHdr;

typedef struct sRequestValue {
	/// \see eArgumentTypes
	uint16_t	Type;
	uint16_t	Flags;
	uint32_t	Length;
}	tRequestValue;

typedef struct sRequestHeader {
	uint32_t	ClientID;
	uint32_t	MessageLength;
	uint16_t	CallID;	//!< \see eSyscalls
	uint16_t	NParams;
	
	tRequestValue	Params[];
} __attribute__((packed))	tRequestHeader;


enum eSyscalls {
	#define _(n) n
	#include "syscalls_list.h"
	#undef _
	N_SYSCALLS
};

#ifndef DONT_INCLUDE_SYSCALL_NAMES
static const char * casSYSCALL_NAMES[] = {
	#define _(n) #n
	#include "syscalls_list.h"
	#undef _
	"-"
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

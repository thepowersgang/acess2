/*
 * Acess2 Native Kernel
 * - Acess kernel emulation on another OS using SDL and UDP
 *
 * Syscall Server
 */
#include <acess.h>
#include "../syscalls.h"

// === TYPES ===
typedef int	(*tSyscallHandler)(const char *Format, void *Args);

// === MACROS ===
#define SYSCALL3(_name, _call, _fmtstr, _t1, _t2, _t3) int _name(const char *fmt,void*args){\
	_t1 a1;_t2 a2;_t3 a3;\
	if(strcmp(fmt,_fmtstr)!=0)return 0;\
	a1 = *(_t1*)args;args+=sizeof(_t1);\
	a2 = *(_t2*)args;args+=sizeof(_t2);\
	a3 = *(_t3*)args;args+=sizeof(_t3);\
	return _call(a1,a2,a3);\
}

#define SYSCALL2(_name, _call, _fmtstr, _t1, _t2) int _name(const char *fmt,void*args){\
	_t1 a1;_t2 a2;\
	if(strcmp(fmt,_fmtstr)!=0)return 0;\
	a1 = *(_t1*)args;args+=sizeof(_t1);\
	a2 = *(_t2*)args;args+=sizeof(_t2);\
	return _call(a1,a2);\
}

#define SYSCALL1V(_name, _call, _fmtstr, _t1) int _name(const char *fmt, void*args){\
	_t1 a1;\
	if(strcmp(fmt,_fmtstr)!=0)return 0;\
	a1 = *(_t1*)args;args+=sizeof(_t1);\
	_call(a1);\
	return 0;\
}

// === CODE ===
int Syscall_Null(const char *Format, void *Args)
{
	return 0;
}

SYSCALL2(Syscall_Open, VFS_Open, "di", const char *, int);
SYSCALL1V(Syscall_Close, VFS_Close, "i", int);
SYSCALL3(Syscall_Read, VFS_Read, "iid", int, int, void *);
SYSCALL3(Syscall_Write, VFS_Write, "iid", int, int, const void *);


const tSyscallHandler	caSyscalls[] = {
	Syscall_Null,
	Syscall_Open,
	Syscall_Close,
	Syscall_Read,
	Syscall_Write
};
const int	ciNumSyscalls = sizeof(caSyscalls)/sizeof(caSyscalls[0]);
/**
 * \brief Recieve a syscall structure from the server code
 */
tRequestHeader *SyscallRecieve(tRequestHeader *Request)
{
	char	formatString[Request->NParams+1];
	char	*inData = (char*)&Request->Params[Request->NParams+Request->NReturn];
	 int	argListLen = 0;
	 int	i, retVal;
	
	if( Request->CallID > ciNumSyscalls ) {
		return NULL;
	}
	
	// Get size of argument list
	for( i = 0; i < Request->NParams; i ++ )
	{
		switch(Request->Params[i].Type)
		{
		case ARG_TYPE_VOID:
			formatString[i] = '-';
			break;
		case ARG_TYPE_INT32:
			formatString[i] = 'i';
			argListLen += sizeof(Uint32);
			break;
		case ARG_TYPE_INT64:
			formatString[i] = 'I';
			argListLen += sizeof(Uint64);
			break;
		case ARG_TYPE_DATA:
			formatString[i] = 'd';
			argListLen += sizeof(void*);
			break;
		default:
			return NULL;	// ERROR!
		}
	}
	
	{
		char	argListData[argListLen];
		argListLen = 0;
		// Build argument list
		for( i = 0; i < Request->NParams; i ++ )
		{
			switch(Request->Params[i].Type)
			{
			case ARG_TYPE_VOID:
				break;
			case ARG_TYPE_INT32:
				*(Uint32*)&argListData[argListLen] = *(Uint32*)inData;
				argListLen += sizeof(Uint32);
				inData += sizeof(Uint32);
				break;
			case ARG_TYPE_INT64:
				*(Uint64*)&argListData[argListLen] = *(Uint64*)inData;
				argListLen += sizeof(Uint64);
				inData += sizeof(Uint64);
				break;
			case ARG_TYPE_DATA:
			case ARG_TYPE_STRING:
				*(void**)&argListData[argListLen] = *(void**)inData;
				argListLen += sizeof(void*);
				inData += sizeof(void*);
				break;
			}
		}
		
		retVal = caSyscalls[Request->CallID](formatString, argListData);
	}
	
	return NULL;
}

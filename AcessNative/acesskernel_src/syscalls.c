/*
 * Acess2 Native Kernel
 * - Acess kernel emulation on another OS using SDL and UDP
 *
 * Syscall Distribution
 */
#include <acess.h>
#include "../syscalls.h"

// === TYPES ===
typedef int	(*tSyscallHandler)(const char *Format, void *Args, int *Sizes);

// === MACROS ===
#define SYSCALL3(_name, _fmtstr, _t0, _t1, _t2, _call) int _name(const char*Fmt,void*Args,int*Sizes){\
	_t0 a0;_t1 a1;_t2 a2;\
	if(strcmp(Fmt,_fmtstr)!=0)return 0;\
	a0 = *(_t0*)Args;Args+=sizeof(_t0);\
	a1 = *(_t1*)Args;Args+=sizeof(_t1);\
	a2 = *(_t2*)Args;Args+=sizeof(_t2);\
	_call\
}

#define SYSCALL2(_name, _fmtstr, _t0, _t1, _call) int _name(const char*Fmt,void*Args,int*Sizes){\
	_t0 a0;_t1 a1;\
	if(strcmp(Fmt,_fmtstr)!=0)return 0;\
	a0 = *(_t0*)Args;Args+=sizeof(_t0);\
	a1 = *(_t1*)Args;Args+=sizeof(_t1);\
	_call;\
}

#define SYSCALL1(_name, _fmtstr, _t0, _call) int _name(const char*Fmt, void*Args,int*Sizes){\
	_t0 a0;\
	if(strcmp(Fmt,_fmtstr)!=0)return 0;\
	a0 = *(_t0*)Args;Args+=sizeof(_t0);\
	_call;\
}

// === CODE ===
int Syscall_Null(const char *Format, void *Args, int *Sizes)
{
	return 0;
}

SYSCALL2(Syscall_Open, "si", const char *, int,
	return VFS_Open(a0, a1|VFS_OPENFLAG_USER);
);
SYSCALL1(Syscall_Close, "i", int,
	VFS_Close(a0);
	return 0;
);
SYSCALL3(Syscall_Read, "iid", int, int, void *,
	if( Sizes[2] <= a1 )
		return -1;
	return VFS_Read(a0, a1, a2);
);
SYSCALL3(Syscall_Write, "iid", int, int, const void *,
	if( Sizes[2] <= a1 )
		return -1;
	return VFS_Write(a0, a1, a2);
);
SYSCALL3(Syscall_Seek, "iIi", int, int64_t, int,
	return VFS_Seek(a0, a1, a2);
);
SYSCALL1(Syscall_Tell, "i", int,
	return VFS_Tell(a0);
);
SYSCALL3(Syscall_IOCtl, "iid", int, int, void *,
	return VFS_IOCtl(a0, a1, a2);
);
SYSCALL3(Syscall_FInfo, "idi", int, void *, int,
	if( Sizes[1] < sizeof(tFInfo)+a2*sizeof(tVFS_ACL))
		return -1;
	return VFS_FInfo(a0, a1, a2);
);


const tSyscallHandler	caSyscalls[] = {
	Syscall_Null,
	Syscall_Open,
	Syscall_Close,
	Syscall_Read,
	Syscall_Write,
	Syscall_Seek,
	Syscall_Tell,
	Syscall_IOCtl,
	Syscall_FInfo
};
const int	ciNumSyscalls = sizeof(caSyscalls)/sizeof(caSyscalls[0]);
/**
 * \brief Recieve a syscall structure from the server code
 */
tRequestHeader *SyscallRecieve(tRequestHeader *Request, int *ReturnLength)
{
	char	formatString[Request->NParams+1];
	char	*inData = (char*)&Request->Params[Request->NParams];
	 int	argListLen = 0;
	 int	i, retVal;
	tRequestHeader	*ret;
	 int	retValueCount = 1;
	 int	retDataLen = sizeof(Uint64);
	void	*returnData[Request->NParams];
	 int	argSizes[Request->NParams];
	
	// Sanity check
	if( Request->CallID >= ciNumSyscalls ) {
		Log_Notice("Syscalls", "Unknown syscall number %i", Request->CallID);
		return NULL;
	}
	
	// Get size of argument list
	for( i = 0; i < Request->NParams; i ++ )
	{
		argSizes[i] = Request->Params[i].Length;
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
		case ARG_TYPE_STRING:
			formatString[i] = 's';
			argListLen += sizeof(char*);
			break;
		default:
			return NULL;	// ERROR!
		}
	}
	formatString[i] = '\0';
	
	Log_Debug("Syscalls", "Request %i '%s'", Request->CallID, formatString);
	
	{
		char	argListData[argListLen];
		argListLen = 0;
		// Build argument list
		for( i = 0; i < Request->NParams; i ++ )
		{
			returnData[i] = NULL;
			switch(Request->Params[i].Type)
			{
			case ARG_TYPE_VOID:
				break;
			case ARG_TYPE_INT32:
				Log_Debug("Syscalls", "Arg %i: 0x%x", i, *(Uint32*)inData);
				*(Uint32*)&argListData[argListLen] = *(Uint32*)inData;
				argListLen += sizeof(Uint32);
				inData += sizeof(Uint32);
				break;
			case ARG_TYPE_INT64:
				Log_Debug("Syscalls", "Arg %i: 0x%llx", i, *(Uint64*)inData);
				*(Uint64*)&argListData[argListLen] = *(Uint64*)inData;
				argListLen += sizeof(Uint64);
				inData += sizeof(Uint64);
				break;
			case ARG_TYPE_STRING:
				Log_Debug("Syscalls", "Arg %i: '%s'", i, (char*)inData);
				*(char**)&argListData[argListLen] = (char*)inData;
				argListLen += sizeof(void*);
				inData += Request->Params[i].Length;
				break;
			
			// Data gets special handling, because only it can be returned to the user
			// (ARG_TYPE_DATA is a pointer)
			case ARG_TYPE_DATA:
				// Prepare the return values
				if( Request->Params[i].Flags & ARG_FLAG_RETURN )
				{
					retDataLen += Request->Params[i].Length;
					retValueCount ++;
				}
				
				// Check for non-resident data
				if( Request->Params[i].Flags & ARG_FLAG_ZEROED )
				{
					// Allocate and zero the buffer
					returnData[i] = calloc(1, Request->Params[i].Length);
					Log_Debug("Syscalls", "Arg %i: %i %p", i,
						Request->Params[i].Length, returnData[i]);
					*(void**)&argListData[argListLen] = returnData[i];
					argListLen += sizeof(void*);
				}
				else
				{
					returnData[i] = (void*)inData;
					Log_Debug("Syscalls", "Arg %i: %i %p", i,
						Request->Params[i].Length, returnData[i]);
					*(void**)&argListData[argListLen] = (void*)inData;
					argListLen += sizeof(void*);
					inData += Request->Params[i].Length;
				}
				break;
			}
		}
		
		retVal = caSyscalls[Request->CallID](formatString, argListData, argSizes);
	}
	
	// Allocate the return
	ret = malloc(sizeof(tRequestHeader) + retValueCount * sizeof(tRequestValue)
		+ retDataLen);
	ret->ClientID = Request->ClientID;
	ret->CallID = Request->CallID;
	ret->NParams = retValueCount;
	inData = (char*)&ret->Params[ ret->NParams ];
	
	// Static Uint64 return value
	ret->Params[0].Type = ARG_TYPE_INT64;
	ret->Params[0].Flags = 0;
	ret->Params[0].Length = sizeof(Uint64);
	*(Uint64*)inData = retVal;
	inData += sizeof(Uint64);
	
	Log_Debug("Syscalls", "Return 0x%llx", retVal);
	
	for( i = 0; i < Request->NParams; i ++ )
	{
		if( Request->Params[i].Type != ARG_TYPE_DATA )	continue;
		if( !(Request->Params[i].Flags & ARG_FLAG_RETURN) )	continue;
		
		ret->Params[1 + i].Type = Request->Params[i].Type;
		ret->Params[1 + i].Flags = 0;
		ret->Params[1 + i].Length = Request->Params[i].Length;
		
		memcpy(inData, returnData[i], Request->Params[i].Length);
		inData += Request->Params[i].Length;
		
		if( Request->Params[i].Flags & ARG_FLAG_ZEROED )
			free( returnData[i] );	// Free temp buffer from above
	}
	
	*ReturnLength = sizeof(tRequestHeader)
		+ retValueCount * sizeof(tRequestValue)
		+ retDataLen;
	
	return ret;
}

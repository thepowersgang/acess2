/*
 * Acess2 Native Kernel
 * - Acess kernel emulation on another OS using SDL and UDP
 *
 * Syscall Distribution
 */
#define DEBUG	1
#include <acess.h>
#include <threads.h>
#include <events.h>
#if DEBUG == 0
# define DONT_INCLUDE_SYSCALL_NAMES
#endif
#include "../syscalls.h"

// === IMPORTS ===
extern int	Threads_Fork(void);	// AcessNative only function
extern int	Threads_Spawn(int nFD, int FDs[], const void *info);

// === TYPES ===
typedef int	(*tSyscallHandler)(Uint *Errno, const char *Format, void *Args, int *Sizes);

// === MACROS ===
#define SYSCALL6(_name, _fmtstr, _t0, _t1, _t2, _t3, _t4, _t5, _call) int _name(Uint*Errno,const char*Fmt,void*Args,int*Sizes){\
	_t0 a0;_t1 a1;_t2 a2;_t3 a3;_t4 a4;_t5 a5;\
	if(strcmp(Fmt,_fmtstr)!=0)return 0;\
	a0 = *(_t0*)Args;Args+=sizeof(_t0);\
	a1 = *(_t1*)Args;Args+=sizeof(_t1);\
	a2 = *(_t2*)Args;Args+=sizeof(_t2);\
	a3 = *(_t3*)Args;Args+=sizeof(_t3);\
	a4 = *(_t4*)Args;Args+=sizeof(_t4);\
	a5 = *(_t5*)Args;Args+=sizeof(_t5);\
	LOG("SYSCALL6 '%s' %p %p %p %p %p %p", Fmt, (intptr_t)a0,(intptr_t)a1,(intptr_t)a2,(intptr_t)a3,(intptr_t)a4,(intptr_t)a5);\
	_call\
}
#define SYSCALL5(_name, _fmtstr, _t0, _t1, _t2, _t3, _t4, _call) int _name(Uint*Errno,const char*Fmt,void*Args,int*Sizes){\
	_t0 a0;_t1 a1;_t2 a2;_t3 a3;_t4 a4;\
	if(strcmp(Fmt,_fmtstr)!=0)return 0;\
	a0 = *(_t0*)Args;Args+=sizeof(_t0);\
	a1 = *(_t1*)Args;Args+=sizeof(_t1);\
	a2 = *(_t2*)Args;Args+=sizeof(_t2);\
	a3 = *(_t3*)Args;Args+=sizeof(_t3);\
	a4 = *(_t4*)Args;Args+=sizeof(_t4);\
	LOG("SYSCALL5 '%s' %p %p %p %p %p", Fmt, (intptr_t)a0,(intptr_t)a1,(intptr_t)a2,(intptr_t)a3,(intptr_t)a4);\
	_call\
}
#define SYSCALL4(_name, _fmtstr, _t0, _t1, _t2, _t3, _call) int _name(Uint*Errno,const char*Fmt,void*Args,int*Sizes){\
	_t0 a0;_t1 a1;_t2 a2;_t3 a3;\
	if(strcmp(Fmt,_fmtstr)!=0)return 0;\
	a0 = *(_t0*)Args;Args+=sizeof(_t0);\
	a1 = *(_t1*)Args;Args+=sizeof(_t1);\
	a2 = *(_t2*)Args;Args+=sizeof(_t2);\
	a3 = *(_t3*)Args;Args+=sizeof(_t3);\
	LOG("SYSCALL4 '%s' %p %p %p %p", Fmt, (intptr_t)a0,(intptr_t)a1,(intptr_t)a2,(intptr_t)a3);\
	_call\
}

#define SYSCALL3(_name, _fmtstr, _t0, _t1, _t2, _call) int _name(Uint*Errno,const char*Fmt,void*Args,int*Sizes){\
	_t0 a0;_t1 a1;_t2 a2;\
	if(strcmp(Fmt,_fmtstr)!=0)return 0;\
	a0 = *(_t0*)Args;Args+=sizeof(_t0);\
	a1 = *(_t1*)Args;Args+=sizeof(_t1);\
	a2 = *(_t2*)Args;Args+=sizeof(_t2);\
	LOG("SYSCALL3 '%s' %p %p %p", Fmt, (intptr_t)a0,(intptr_t)a1,(intptr_t)a2);\
	_call\
}

#define SYSCALL2(_name, _fmtstr, _t0, _t1, _call) int _name(Uint*Errno,const char*Fmt,void*Args,int*Sizes){\
	_t0 a0;_t1 a1;\
	if(strcmp(Fmt,_fmtstr)!=0)return 0;\
	a0 = *(_t0*)Args;Args+=sizeof(_t0);\
	a1 = *(_t1*)Args;Args+=sizeof(_t1);\
	LOG("SYSCALL2 '%s' %p %p", Fmt, (intptr_t)a0,(intptr_t)a1);\
	_call;\
}

#define SYSCALL1(_name, _fmtstr, _t0, _call) int _name(Uint*Errno,const char*Fmt, void*Args,int*Sizes){\
	_t0 a0;\
	if(strcmp(Fmt,_fmtstr)!=0)return 0;\
	a0 = *(_t0*)Args;Args+=sizeof(_t0);\
	LOG("SYSCALL1 '%s' %p", Fmt,(intptr_t)a0);\
	_call;\
}

#define SYSCALL0(_name, _call) int _name(Uint*Errno,const char*Fmt, void*Args,int*Sizes){\
	if(strcmp(Fmt,"")!=0)return 0;\
	LOG("SYSCALL0");\
	_call;\
}

// === CODE ===
int Syscall_Null(Uint *Errno, const char *Format, void *Args, int *Sizes)
{
	return 0;
}

SYSCALL1(Syscall_Exit, "i", int,
	Threads_Exit(0, a0);
	return 0;
);

SYSCALL2(Syscall_Open, "si", const char *, int,
	int rv = VFS_Open(a0, a1|VFS_OPENFLAG_USER);
	if(rv == -1)	*Errno = errno;
	return rv;
);
SYSCALL1(Syscall_Close, "i", int,
	VFS_Close(a0);
	return 0;
);
SYSCALL3(Syscall_Read, "iid", int, int, void *,
	if( Sizes[2] < a1 ) {
		Log_Warning("Syscalls", "Read - %i < %i", Sizes[2], a1);
		return -1;
	}
	size_t rv = VFS_Read(a0, a1, a2);
	if(rv == -1)	*Errno = errno;
	return rv;
);
SYSCALL3(Syscall_Write, "iid", int, int, const void *,
	if( Sizes[2] < a1 ) {
		Log_Warning("Syscalls", "Write - %x < %x", (int)Sizes[2], (int)a1);
		*Errno = EINVAL;
		return -1;
	}
	size_t rv = VFS_Write(a0, a1, a2);
	if(rv == -1)	*Errno = errno;
	return rv;
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
	if( Sizes[1] < sizeof(tFInfo)+a2*sizeof(tVFS_ACL)) {
		//LOG("offsetof(size) = %i", offsetof(tFInfo, size));
		LOG("Bad size %i < %i", Sizes[1], sizeof(tFInfo)+a2*sizeof(tVFS_ACL));
		*Errno = -EINVAL;
		return -1;
	}
	return VFS_FInfo(a0, a1, a2);
);
SYSCALL2(Syscall_ReadDir, "id", int, char *,
	if(Sizes[1] < 255)
		return -1;
	return VFS_ReadDir(a0, a1);
);
SYSCALL6(Syscall_select, "iddddi", int, fd_set *, fd_set *, fd_set *, tTime *, unsigned int,
	return VFS_Select(a0, a1, a2, a3, a4, a5, 0);
);
SYSCALL3(Syscall_OpenChild, "isi", int, const char *, int,
	return VFS_OpenChild(a0, a1, a2|VFS_OPENFLAG_USER);
);
SYSCALL2(Syscall_GetACL, "id", int, void *,
	if(Sizes[1] < sizeof(tVFS_ACL))
		return -1;
	return VFS_GetACL(a0, (void*)a1);
);
SYSCALL4(Syscall_Mount, "ssss", const char *, const char *, const char *, const char *,
	return VFS_Mount(a0, a1, a2, a3);
);
SYSCALL1(Syscall_Chdir, "s", const char *,
	return VFS_ChDir(a0);
);
SYSCALL0(Syscall_Sleep,
	Threads_Sleep();
	return 0;
);
SYSCALL2(Syscall_WaitTID, "id", int, int *,
	if(Sizes[1] < sizeof(int))
		return -1;
	return Threads_WaitTID(a0, a1);
);
SYSCALL1(Syscall_SetUID, "i", int,
	if(Sizes[0] < sizeof(int)) {
		*Errno = -EINVAL;	// TODO: Better message
		return -1;
	}
	return Threads_SetUID(a0);
);
SYSCALL1(Syscall_SetGID, "i", int,
	if(Sizes[0] < sizeof(int)) {
		*Errno = -EINVAL;	// TODO: Better message
		return -1;
	}
	return Threads_SetGID(a0);
);

SYSCALL0(Syscall_GetTID, return Threads_GetTID());
SYSCALL0(Syscall_GetPID, return Threads_GetPID());
SYSCALL0(Syscall_GetUID, return Threads_GetUID());
SYSCALL0(Syscall_GetGID, return Threads_GetGID());

SYSCALL1(Syscall_AN_Fork, "d", int *,
	if(Sizes[0] < sizeof(int))
		return -1;
	*a0 = Threads_Fork();
	return *a0;
);

SYSCALL3(Syscall_AN_Spawn, "ddd", int *, int *, void *,
	if(Sizes[0] < sizeof(int))
		return -1;
	*a0 = Threads_Spawn(Sizes[1] / sizeof(int), a1, a2);
	return *a0;
);

SYSCALL2(Syscall_SendMessage, "id", int, void *,
	return Proc_SendMessage(a0, Sizes[1], a1);
);

SYSCALL2(Syscall_GetMessage, "dd", uint32_t *, void *,
	if( a0 && Sizes[0] < sizeof(*a0) ) {
		Log_Notice("Syscalls", "Syscall_GetMessage - Arg 1 Undersize (%i < %i)",
			Sizes[0], sizeof(*a0));
		return -1;
	}
	Uint	tmp;
	 int	rv;
	if( a0 ) {
		rv = Proc_GetMessage(&tmp, Sizes[1], a1);
		*a0 = tmp;
	}
	else
		rv = Proc_GetMessage(NULL, Sizes[1], a1);
	return rv;
);

SYSCALL1(Syscall_WaitEvent, "i", int,
	return Threads_WaitEvents(a0);
);

const tSyscallHandler	caSyscalls[] = {
	[SYS_NULL]	= Syscall_Null,
	[SYS_EXIT]	= Syscall_Exit,
	[SYS_OPEN]	= Syscall_Open,
	[SYS_CLOSE]	= Syscall_Close,
	[SYS_COPYFD]	= NULL,
	[SYS_FDFLAGS]	= NULL,
	[SYS_READ]	= Syscall_Read,
	[SYS_WRITE]	= Syscall_Write,
	[SYS_SEEK]	= Syscall_Seek,
	[SYS_TELL]	= Syscall_Tell,
	[SYS_IOCTL]	= Syscall_IOCtl,
	[SYS_FINFO]	= Syscall_FInfo,
	[SYS_READDIR]	= Syscall_ReadDir,
	[SYS_OPENCHILD]	= Syscall_OpenChild,
	[SYS_GETACL]	= Syscall_GetACL,
	[SYS_MOUNT]	= Syscall_Mount,
	[SYS_REOPEN]	= NULL,	// SYS_REOPEN
	[SYS_CHDIR]	= Syscall_Chdir,
	
	[SYS_WAITTID]	= Syscall_WaitTID,
	[SYS_SETUID]	= Syscall_SetUID,
	[SYS_SETGID]	= Syscall_SetGID,
	
	Syscall_GetTID,
	Syscall_GetPID,
	Syscall_GetUID,
	Syscall_GetGID,

	Syscall_Sleep,
	Syscall_AN_Fork,
	Syscall_AN_Spawn,

	Syscall_SendMessage,
	Syscall_GetMessage,
	Syscall_select,
	Syscall_WaitEvent
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
	 int	retValueCount;
	 int	retDataLen;
	void	*returnData[Request->NParams];
	 int	argSizes[Request->NParams];
	Uint	ret_errno = 0;
	
	// Clear errno (Acess verson) at the start of the request
	errno = 0;
	
	// Sanity check
	if( Request->CallID >= ciNumSyscalls ) {
		Log_Notice("Syscalls", "Unknown syscall number %i", Request->CallID);
		return NULL;
	}
	
	if( !caSyscalls[Request->CallID] ) {
		Log_Notice("Syscalls", "Unimplemented syscall %i", Request->CallID);
		return NULL;
	}

	// Init return count/size
	retValueCount = 2;
	retDataLen = sizeof(Uint64) + sizeof(Uint32);	

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
			// Prepare the return values
			if( Request->Params[i].Flags & ARG_FLAG_RETURN )
			{
				retDataLen += Request->Params[i].Length;
				retValueCount ++;
			}
			break;
		case ARG_TYPE_STRING:
			formatString[i] = 's';
			argListLen += sizeof(char*);
			break;
		default:
			Log_Error("Syscalls", "Unknown param type %i", Request->Params[i].Type);
			return NULL;	// ERROR!
		}
	}
	formatString[i] = '\0';
	
	LOG("Request %i(%s) '%s'", Request->CallID, casSYSCALL_NAMES[Request->CallID], formatString);
	
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
				//LOG("%i INT32: 0x%x", i, *(Uint32*)inData);
				*(Uint32*)&argListData[argListLen] = *(Uint32*)inData;
				argListLen += sizeof(Uint32);
				inData += sizeof(Uint32);
				break;
			case ARG_TYPE_INT64:
				//LOG("%i INT64: 0x%llx", i, *(Uint64*)inData);
				*(Uint64*)&argListData[argListLen] = *(Uint64*)inData;
				argListLen += sizeof(Uint64);
				inData += sizeof(Uint64);
				break;
			case ARG_TYPE_STRING:
				//LOG("%i STR: '%s'", i, (char*)inData);
				*(char**)&argListData[argListLen] = (char*)inData;
				argListLen += sizeof(void*);
				inData += Request->Params[i].Length;
				break;
			
			// Data gets special handling, because only it can be returned to the user
			// (ARG_TYPE_DATA is a pointer)
			case ARG_TYPE_DATA:
				// Check for non-resident data
				if( Request->Params[i].Length == 0 )
				{
					returnData[i] = NULL;
					*(void**)&argListData[argListLen] = NULL;
					argListLen += sizeof(void*);
				}
				else if( Request->Params[i].Flags & ARG_FLAG_ZEROED )
				{
					// Allocate and zero the buffer
					returnData[i] = calloc(1, Request->Params[i].Length);
					//LOG("%i ZDAT: %i %p", i,
					//	Request->Params[i].Length, returnData[i]);
					*(void**)&argListData[argListLen] = returnData[i];
					argListLen += sizeof(void*);
				}
				else
				{
					returnData[i] = (void*)inData;
					//LOG("%i DATA: %i %p", i,
					//	Request->Params[i].Length, returnData[i]);
					*(void**)&argListData[argListLen] = (void*)inData;
					argListLen += sizeof(void*);
					inData += Request->Params[i].Length;
				}
				break;
			}
		}
		
		// --- Perform request
		retVal = caSyscalls[Request->CallID](&ret_errno, formatString, argListData, argSizes);
	}
	
	// ---------- Return
	
	if( ret_errno == 0 && errno != 0 ) {
		ret_errno = errno;
		LOG("errno = %i", errno);
	}
	
	// Allocate the return
	size_t	msglen = sizeof(tRequestHeader) + retValueCount * sizeof(tRequestValue) + retDataLen;
	ret = malloc(msglen);
	ret->ClientID = Request->ClientID;
	ret->CallID = Request->CallID;
	ret->NParams = retValueCount;
	ret->MessageLength = msglen;
	inData = (char*)&ret->Params[ ret->NParams ];
	
	// Static Uint64 return value
	ret->Params[0].Type = ARG_TYPE_INT64;
	ret->Params[0].Flags = 0;
	ret->Params[0].Length = sizeof(Uint64);
	*(Uint64*)inData = retVal;
	inData += sizeof(Uint64);
	
	// Static Uint32 errno value
	ret->Params[1].Type = ARG_TYPE_INT32;
	ret->Params[1].Flags = 0;
	ret->Params[1].Length = sizeof(Uint32);
	*(Uint32*)inData = ret_errno;
	inData += sizeof(Uint32);

	LOG("Ret: %llx, errno=%i", retVal, ret_errno);	

	//Log_Debug("Syscalls", "Return 0x%llx", retVal);
	
	retValueCount = 2;
	for( i = 0; i < Request->NParams; i ++ )
	{
		if( Request->Params[i].Type != ARG_TYPE_DATA )	continue;
		if( !(Request->Params[i].Flags & ARG_FLAG_RETURN) )	continue;
		
		ret->Params[retValueCount].Type = Request->Params[i].Type;
		ret->Params[retValueCount].Flags = 0;
		ret->Params[retValueCount].Length = Request->Params[i].Length;
		
		LOG("Ret %i: Type %i, Len %i",
			i, Request->Params[i].Type, Request->Params[i].Length);
		
		memcpy(inData, returnData[i], Request->Params[i].Length);
		inData += Request->Params[i].Length;
		
		if( Request->Params[i].Flags & ARG_FLAG_ZEROED )
			free( returnData[i] );	// Free temp buffer from above
		retValueCount ++;
	}
	
	*ReturnLength = ret->MessageLength;
	
	return ret;
}

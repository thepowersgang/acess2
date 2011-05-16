/*
 */
#define DONT_INCLUDE_SYSCALL_NAMES 1
#include "../../Usermode/include/acess/sys.h"
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <stddef.h>
#include "request.h"
#include "../syscalls.h"

#define DEBUG(x...)	printf(x)

#define NATIVE_FILE_MASK	0x40000000
#define	MAX_FPS	16

// === Types ===

// === IMPORTS ===
extern int	giSyscall_ClientID;	// Needed for execve

// === GLOBALS ===
FILE	*gaSyscall_LocalFPs[MAX_FPS];

// === CODE ===
const char *ReadEntry(tRequestValue *Dest, void *DataDest, void **PtrDest, const char *ArgTypes, va_list *Args)
{
	uint64_t	val64;
	uint32_t	val32;
	 int	direction = 0;	// 0: Invalid, 1: Out, 2: In, 3: Out
	char	*str;
	 int	len;
	
	// Eat whitespace
	while(*ArgTypes && *ArgTypes == ' ')	ArgTypes ++;
	if( *ArgTypes == '\0' )	return ArgTypes;
	
	// Get direction
	switch(*ArgTypes)
	{
	default:	// Defaults to output
	case '>':	direction = 1;	break;
	case '<':	direction = 2;	break;
	case '?':	direction = 3;	break;
	}
	ArgTypes ++;
	
	// Eat whitespace
	while(*ArgTypes && *ArgTypes == ' ')	ArgTypes ++;
	if( *ArgTypes == '\0' )	return ArgTypes;
	
	// Get type
	switch(*ArgTypes)
	{
	// 32-bit integer
	case 'i':
		
		if( direction != 1 ) {
			fprintf(stderr, "ReadEntry: Recieving an integer is not defined\n");
			return NULL;
		}
		
		val32 = va_arg(*Args, uint32_t);
		
		Dest->Type = ARG_TYPE_INT32;
		Dest->Length = sizeof(uint32_t);
		Dest->Flags = 0;
		
		if( DataDest )
			*(uint32_t*)DataDest = val32;
		break;
	// 64-bit integer
	case 'I':
		
		if( direction != 1 ) {
			fprintf(stderr, "ReadEntry: Recieving an integer is not defined\n");
			return NULL;
		}
		
		val64 = va_arg(*Args, uint64_t);
		
		Dest->Type = ARG_TYPE_INT64;
		Dest->Length = sizeof(uint64_t);
		Dest->Flags = 0;
		if( DataDest )
			*(uint64_t*)DataDest = val64;
		break;
	// String
	case 's':
		// Input string makes no sense!
		if( direction != 1 ) {
			fprintf(stderr, "ReadEntry: Recieving a string is not defined\n");
			return NULL;
		}
		
		str = va_arg(*Args, char*);
		
		Dest->Type = ARG_TYPE_STRING;
		Dest->Length = strlen(str) + 1;
		Dest->Flags = 0;
		
		if( DataDest )
		{
			memcpy(DataDest, str, Dest->Length);
		}
		break;
	// Data (special handling)
	case 'd':
		len = va_arg(*Args, int);
		str = va_arg(*Args, char*);
		
		// Save the pointer for later
		if( PtrDest )	*PtrDest = str;
		
		// Create parameter block
		Dest->Type = ARG_TYPE_DATA;
		Dest->Length = len;
		Dest->Flags = 0;
		if( direction & 2 )
			Dest->Flags |= ARG_FLAG_RETURN;
		
		// Has data?
		if( direction & 1 )
		{
			if( DataDest )
				memcpy(DataDest, str, len);
		}
		else
			Dest->Flags |= ARG_FLAG_ZEROED;
		break;
	
	default:
		return NULL;
	}
	ArgTypes ++;
	
	return ArgTypes;
}

/**
 * \param ArgTypes
 *
 * Whitespace is ignored
 * >i:	Input Integer (32-bits)
 * >I:	Input Long Integer (64-bits)
 * >s:	Input String
 * >d:	Input Buffer (Preceded by valid size)
 * <I:	Output long integer
 * <d:	Output Buffer (Preceded by valid size)
 * ?d:  Bi-directional buffer (Preceded by valid size), buffer contents
 *      are returned
 */
uint64_t _Syscall(int SyscallID, const char *ArgTypes, ...)
{
	va_list	args;
	 int	paramCount, dataLength;
	 int	retCount = 1, retLength = sizeof(uint64_t);
	void	**retPtrs;	// Pointers to return buffers
	const char	*str;
	tRequestHeader	*req;
	void	*dataPtr;
	uint64_t	retValue;
	 int	i;
	
	// DEBUG!
//	printf("&tRequestHeader->Params = %i\n", offsetof(tRequestHeader, Params));
//	printf("&tRequestValue->Flags = %i\n", offsetof(tRequestValue, Flags));
//	printf("&tRequestValue->Length = %i\n", offsetof(tRequestValue, Length));
	
	// Get data size
	va_start(args, ArgTypes);
	str = ArgTypes;
	paramCount = 0;
	dataLength = 0;
	while(*str)
	{
		tRequestValue	tmpVal;
		
		str = ReadEntry(&tmpVal, NULL, NULL, str, &args);
		if( !str ) {
			fprintf(stderr, "syscalls.c: ReadEntry failed (SyscallID = %i)\n", SyscallID);
			exit(127);
		}
		paramCount ++;
		if( !(tmpVal.Flags & ARG_FLAG_ZEROED) )
			dataLength += tmpVal.Length;
		
		if( tmpVal.Flags & ARG_FLAG_RETURN ) {
			retLength += tmpVal.Length;
			retCount ++;
		}
	}
	va_end(args);
	
	dataLength += sizeof(tRequestHeader) + paramCount*sizeof(tRequestValue);
	retLength += sizeof(tRequestHeader) + retCount*sizeof(tRequestValue);
	
	// Allocate buffers
	retPtrs = malloc( sizeof(void*) * (retCount+1) );
	if( dataLength > retLength)
		req = malloc( dataLength );
	else
		req = malloc( retLength );
	req->ClientID = 0;	//< Filled later
	req->CallID = SyscallID;
	req->NParams = paramCount;
	dataPtr = &req->Params[paramCount];
	
	// Fill `output` and `input`
	va_start(args, ArgTypes);
	str = ArgTypes;
	// - re-zero so they can be used as indicies
	paramCount = 0;
	retCount = 0;
	while(*str)
	{		
		str = ReadEntry(&req->Params[paramCount], dataPtr, &retPtrs[retCount], str, &args);
		if( !str )	break;
		
		if( !(req->Params[paramCount].Flags & ARG_FLAG_ZEROED) )
			dataPtr += req->Params[paramCount].Length;
		if( req->Params[paramCount].Flags & ARG_FLAG_RETURN )
			retCount ++;
		
		paramCount ++;
	}
	va_end(args);
	
	// Send syscall request
	if( SendRequest(req, dataLength, retLength) < 0 ) {
		fprintf(stderr, "syscalls.c: SendRequest failed (SyscallID = %i)\n", SyscallID);
		exit(127);
	}
	
	// Parse return value
	dataPtr = &req->Params[req->NParams];
	retValue = 0;
	if( req->NParams >= 1 )
	{
		switch(req->Params[0].Type)
		{
		case ARG_TYPE_INT64:
			retValue = *(uint64_t*)dataPtr;
			dataPtr += req->Params[0].Length;
			break;
		case ARG_TYPE_INT32:
			retValue = *(uint32_t*)dataPtr;
			dataPtr += req->Params[0].Length;
			break;
		}	
	}
	
	// Write changes to buffers
	retCount = 0;
	for( i = 1; i < req->NParams; i ++ )
	{
		#if 0
		 int	 j;
		printf("Return Data %i: (%i)", i, req->Params[i].Length);
		for( j = 0; j < req->Params[i].Length; j ++ )
			printf(" %02x", ((uint8_t*)dataPtr)[j]);
		printf("\n");
		#endif
		memcpy( retPtrs[retCount++], dataPtr, req->Params[i].Length );
		dataPtr += req->Params[i].Length;
	}
	
	free( req );
	free( retPtrs );
	
	DEBUG(": %llx\n", retValue);
	
	return retValue;
}

// --- VFS Calls
int acess_open(const char *Path, int Flags)
{
	if( strncmp(Path, "$$$$", 4) == 0 )
	{
		 int	ret;
		for(ret = 0; ret < MAX_FPS && gaSyscall_LocalFPs[ret]; ret ++ )	;
		if(ret == MAX_FPS)	return -1;
		// TODO: Handle directories
		gaSyscall_LocalFPs[ret] = fopen(&Path[4], "r+");
		if(!gaSyscall_LocalFPs[ret])	return -1;
		return ret|NATIVE_FILE_MASK;
	}
	DEBUG("open(\"%s\", 0x%x)", Path, Flags);
	return _Syscall(SYS_OPEN, ">s >i", Path, Flags);
}

void acess_close(int FD) {
	if(FD & NATIVE_FILE_MASK) {
		fclose( gaSyscall_LocalFPs[FD & (NATIVE_FILE_MASK-1)] );
		gaSyscall_LocalFPs[FD & (NATIVE_FILE_MASK-1)] = NULL;
		return ;
	}
	DEBUG("close(%i)", FD);
	_Syscall(SYS_CLOSE, ">i", FD);
}

int acess_reopen(int FD, const char *Path, int Flags) {
	DEBUG("reopen(0x%x, \"%s\", 0x%x)", FD, Path, Flags);
	return _Syscall(SYS_REOPEN, ">i >s >i", FD, Path, Flags);
}

size_t acess_read(int FD, size_t Bytes, void *Dest) {
	if(FD & NATIVE_FILE_MASK)
		return fread( Dest, Bytes, 1, gaSyscall_LocalFPs[FD & (NATIVE_FILE_MASK-1)] );
	DEBUG("read(0x%x, 0x%x, *%p)", FD, Bytes, Dest);
	return _Syscall(SYS_READ, ">i >i <d", FD, Bytes, Bytes, Dest);
}

size_t acess_write(int FD, size_t Bytes, void *Src) {
	if(FD & NATIVE_FILE_MASK)
		return fwrite( Src, Bytes, 1, gaSyscall_LocalFPs[FD & (NATIVE_FILE_MASK-1)] );
	DEBUG("write(0x%x, 0x%x, %p\"%.*s\")", FD, Bytes, Src, Bytes, (char*)Src);
	return _Syscall(SYS_WRITE, ">i >i >d", FD, Bytes, Bytes, Src);
}

int acess_seek(int FD, int64_t Ofs, int Dir) {
	if(FD & NATIVE_FILE_MASK) {
		switch(Dir) {
		case ACESS_SEEK_SET:	Dir = SEEK_SET;	break;
		default:
		case ACESS_SEEK_CUR:	Dir = SEEK_CUR;	break;
		case ACESS_SEEK_END:	Dir = SEEK_END;	break;
		}
		return fseek( gaSyscall_LocalFPs[FD & (NATIVE_FILE_MASK-1)], Ofs, Dir );
	}
	DEBUG("seek(0x%x, 0x%llx, %i)", FD, Ofs, Dir);
	return _Syscall(SYS_SEEK, ">i >I >i", FD, Ofs, Dir);
}

uint64_t acess_tell(int FD) {
	if(FD & NATIVE_FILE_MASK)
		return ftell( gaSyscall_LocalFPs[FD & (NATIVE_FILE_MASK-1)] );
	return _Syscall(SYS_TELL, ">i", FD);
}

int acess_ioctl(int fd, int id, void *data) {
	// NOTE: 1024 byte size is a hack
	return _Syscall(SYS_IOCTL, ">i >i ?d", fd, id, 1024, data);
}
int acess_finfo(int fd, t_sysFInfo *info, int maxacls) {
	return _Syscall(SYS_FINFO, ">i <d >i",
		fd,
		sizeof(t_sysFInfo)+maxacls*sizeof(t_sysACL), info,
		maxacls);
}

int acess_readdir(int fd, char *dest) {
	return _Syscall(SYS_READDIR, ">i <d", fd, 256, dest);
}

int acess__SysOpenChild(int fd, char *name, int flags) {
	return _Syscall(SYS_OPENCHILD, ">i >s >i", fd, name, flags);
}

int acess__SysGetACL(int fd, t_sysACL *dest) {
	return _Syscall(SYS_GETACL, "<i >i <d", fd, sizeof(t_sysACL), dest);
}

int acess__SysMount(const char *Device, const char *Directory, const char *Type, const char *Options) {
	return _Syscall(SYS_MOUNT, ">s >s >s >s", Device, Directory, Type, Options);
}


// --- Error Handler
int	acess__SysSetFaultHandler(int (*Handler)(int)) {
	printf("TODO: Set fault handler (asked to set to %p)\n", Handler);
	return 0;
}

// --- Memory Management ---
uint64_t acess__SysAllocate(uint vaddr)
{
	if( AllocateMemory(vaddr, 0x1000) == -1 )	// Allocate a page
		return 0;
		
	return vaddr;	// Just ignore the need for paddrs :)
}

// --- Process Management ---
int acess_clone(int flags, void *stack)
{
	extern int fork(void);
	if(flags & CLONE_VM) {
		 int	ret, newID, kernel_tid=0;
		printf("fork()");
		
		newID = _Syscall(SYS_FORK, "<d", sizeof(int), &kernel_tid);
		ret = fork();
		if(ret < 0)	return ret;
		
		if(ret == 0)
		{
			giSyscall_ClientID = newID;
			return 0;
		}
		
		// TODO: Return the acess TID instead
		return kernel_tid;
	}
	else
	{
		fprintf(stderr, "ERROR: Threads currently unsupported\n");
		exit(-1);
	}
}

int acess_execve(char *path, char **argv, char **envp)
{
	 int	i, argc;
	
	printf("acess_execve: (path='%s', argv=%p, envp=%p)\n", path, argv, envp);
	
	// Get argument count
	for( argc = 0; argv[argc]; argc ++ ) ;
	printf(" acess_execve: argc = %i\n", argc);
	
	char	*new_argv[5+argc+1];
	char	key[11];
	sprintf(key, "%i", giSyscall_ClientID);
	new_argv[0] = "ld-acess";	// TODO: Get path to ld-acess executable
	new_argv[1] = "--key";	// Set socket/client ID for Request.c
	new_argv[2] = key;
	new_argv[3] = "--binary";	// Set the binary path (instead of using argv[0])
	new_argv[4] = path;
	for( i = 0; i < argc; i ++ )	new_argv[5+i] = argv[i];
	new_argv[5+i] = NULL;
	
	#if 1
	argc += 5;
	for( i = 0; i < argc; i ++ )
		printf("\"%s\" ", new_argv[i]);
	printf("\n");
	#endif
	
	// Call actual execve
	return execve("./ld-acess", new_argv, envp);
}

void acess_sleep(void)
{
	_Syscall(SYS_SLEEP, "");
}

int acess_waittid(int TID, int *ExitStatus)
{
	return _Syscall(SYS_WAITTID, ">i <d", TID, sizeof(int), &ExitStatus);
}

int acess_setuid(int ID)
{
	return _Syscall(SYS_SETUID, ">i", ID);
}

int acess_setgid(int ID)
{
	return _Syscall(SYS_SETGID, ">i", ID);
}

// --- Logging
void acess__SysDebug(const char *Format, ...)
{
	va_list	args;
	
	va_start(args, Format);
	
	printf("[_SysDebug] ");
	vprintf(Format, args);
	printf("\n");
	
	va_end(args);
}

void acess__exit(int Status)
{
	_Syscall(SYS_EXIT, ">i", Status);
	exit(Status);
}


// === Symbol List ===
#define DEFSYM(name)	{#name, acess_##name}
const tSym	caBuiltinSymbols[] = {
	DEFSYM(_exit),
	
	DEFSYM(open),
	DEFSYM(close),
	DEFSYM(reopen),
	DEFSYM(read),
	DEFSYM(write),
	DEFSYM(seek),
	DEFSYM(tell),
	DEFSYM(ioctl),
	DEFSYM(finfo),
	DEFSYM(readdir),
	DEFSYM(_SysOpenChild),
	DEFSYM(_SysGetACL),
	DEFSYM(_SysMount),
	
	DEFSYM(clone),
	DEFSYM(execve),
	DEFSYM(sleep),
	
	DEFSYM(waittid),
	DEFSYM(setuid),
	DEFSYM(setgid),
	
	DEFSYM(_SysAllocate),
	DEFSYM(_SysDebug),
	DEFSYM(_SysSetFaultHandler)
};

const int	ciNumBuiltinSymbols = sizeof(caBuiltinSymbols)/sizeof(caBuiltinSymbols[0]);


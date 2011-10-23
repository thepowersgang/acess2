/*
 */
#define DONT_INCLUDE_SYSCALL_NAMES 1
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <stddef.h>
#include "request.h"

#define DEBUG(str, x...)	Debug(str, x)

#define	MAX_FPS	16

// === Types ===

// === IMPORTS ===

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
			Warning("ReadEntry: Recieving an integer is not defined");
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
	
	DEBUG(": %llx", retValue);
	
	return retValue;
}


int native_open(const char *Path, int Flags)
{
	int	ret;
       for(ret = 0; ret < MAX_FPS && gaSyscall_LocalFPs[ret]; ret ++ )	;
       if(ret == MAX_FPS)	return -1;
       // TODO: Handle directories
       gaSyscall_LocalFPs[ret] = fopen(&Path[4], "r+");
       if(!gaSyscall_LocalFPs[ret])	return -1;
       return ret;
}

void native_close(int FD)
{
	fclose( gaSyscall_LocalFPs[FD] );
	gaSyscall_LocalFPs[FD] = NULL;
}

size_t native_read(int FD, void *Dest, size_t Bytes)
{
	return fread( Dest, Bytes, 1, gaSyscall_LocalFPs[FD] );
}

size_t native_write(int FD, const void *Src, size_t Bytes)
{
	return fwrite( Src, Bytes, 1, gaSyscall_LocalFPs[FD] );
}

int native_seek(int FD, int64_t Ofs, int Dir)
{
	if(Dir == 0)
		return fseek( gaSyscall_LocalFPs[FD], Ofs, SEEK_CUR );
	else if(Dir > 0)
		return fseek( gaSyscall_LocalFPs[FD], Ofs, SEEK_SET );
	else
		return fseek( gaSyscall_LocalFPs[FD], Ofs, SEEK_END );
}

uint64_t native_tell(int FD)
{
	return ftell( gaSyscall_LocalFPs[FD] );
}

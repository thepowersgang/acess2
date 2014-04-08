/*
 * Acess2 Kernel
 * 
 * adt.c
 * - Complex data type code
 */
#include <acess.h>
#include <adt.h>


// === CODE ===
// --- Ring Buffers ---
tRingBuffer *RingBuffer_Create(size_t Space)
{
	tRingBuffer	*ret = calloc(1, sizeof(tRingBuffer)+Space);
	ret->Space = Space;
	return ret;
}

void RingBuffer_Free(tRingBuffer *Buffer)
{
	free(Buffer);
}

size_t RingBuffer_Read(void *Dest, tRingBuffer *Buffer, size_t Length)
{
	size_t	tmpLen;

	tmpLen = Buffer->Length;	// Changed in Write, so cache it for our read

	if(Length > tmpLen)	Length = tmpLen;
	
	if( Buffer->Start + Length > Buffer->Space )
	{
		 int	endData = Buffer->Space - Buffer->Start;
		memcpy(Dest, &Buffer->Data[Buffer->Start], endData);
		memcpy((Uint8*)Dest + endData, Buffer->Data, Length - endData);
	}
	else
	{
		memcpy(Dest, &Buffer->Data[Buffer->Start], Length);
	}

	// Lock then modify
	SHORTLOCK( &Buffer->Lock );
	Buffer->Start += Length;
	if( Buffer->Start > Buffer->Space )
		Buffer->Start -= Buffer->Space;
	Buffer->Length -= Length;
	SHORTREL( &Buffer->Lock );

	return Length;
}

size_t RingBuffer_Write(tRingBuffer *Buffer, const void *Source, size_t Length)
{
	size_t	bufEnd, endSpace;
	size_t	tmpLen, tmpStart;
	
	// Cache Start and Length because _Read can change these
	SHORTLOCK( &Buffer->Lock );
	tmpStart = Buffer->Start;
	tmpLen = Buffer->Length;
	SHORTREL( &Buffer->Lock );

	bufEnd = (tmpStart + Buffer->Length) % Buffer->Space;
	endSpace = Buffer->Space - bufEnd;
	
	// Force to bounds
	if(Length > Buffer->Space - tmpLen)	Length = Buffer->Space - tmpLen;
	
	if(endSpace < Length)
	{
		memcpy( &Buffer->Data[bufEnd], Source, endSpace );
		memcpy( Buffer->Data, (Uint8*)Source + endSpace, Length - endSpace );
	}
	else
	{
		memcpy( &Buffer->Data[bufEnd], Source, Length );
	}

	// Lock then modify
	SHORTLOCK( &Buffer->Lock );
	Buffer->Length += Length;
	SHORTREL( &Buffer->Lock );
	
	return Length;
}


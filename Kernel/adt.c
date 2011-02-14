/*
 */
#include <acess.h>
#include <adt.h>

// === CODE ===
// --- Ring Buffers ---
tRingBuffer *RingBuffer_Create(size_t Space)
{
	tRingBuffer	*ret = malloc(sizeof(tRingBuffer)+Space);
	ret->Start = 0;
	ret->Length = 0;
	ret->Space = Space;
	return ret;
}

size_t RingBuffer_Read(void *Dest, tRingBuffer *Buffer, size_t Length)
{
	if(Length > Buffer->Length)	Length = Buffer->Length;
	
	if( Buffer->Start + Length > Buffer->Space )
	{
		 int	endData = Buffer->Space - Buffer->Start;
		memcpy(Dest, &Buffer->Data[Buffer->Start], endData);
		memcpy((Uint8*)Dest + endData, &Buffer->Data, Length - endData);
	}
	else
	{
		memcpy(Dest, &Buffer->Data[Buffer->Start], Length);
	}
	Buffer->Length -= Length;
	return Length;
}

size_t RingBuffer_Write(tRingBuffer *Buffer, void *Source, size_t Length)
{
	size_t	bufEnd = Buffer->Start + Buffer->Length;
	size_t	endSpace = Buffer->Space - bufEnd;
	
	// Force to bounds
	if(Length > Buffer->Space - Buffer->Length)
		Length = Buffer->Space - Buffer->Length;
	
	if(endSpace < Length)
	{
		memcpy( &Buffer->Data[bufEnd], Source, endSpace );
		memcpy( Buffer->Data, (Uint8*)Source + endSpace, Length - endSpace );
		Buffer->Length = Length - endSpace;
	}
	else
	{
		memcpy( &Buffer->Data[bufEnd], Source, Length );
		Buffer->Length += Length;
	}
	
	return Length;
}

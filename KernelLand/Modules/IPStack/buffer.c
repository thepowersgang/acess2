/*
 * Acess2 Networking Stack
 * - By John Hodge (thePowersGang)
 *
 * buffer.c
 * - Scatter-gather handling
 */
#include "ipstack.h"
#include "include/buffer.h"

// === STRUCTURES ===
struct sIPStackBuffer
{
	 int	MaxSubBufffers;
	 int	nSubBuffers;
	size_t	TotalLength;
	struct _subbuffer
	{
		const void	*Data;
		size_t	PreLength;
		size_t	PostLength;
		// TODO: Callbacks?
	} SubBuffers[];
};

// === CODE ===
tIPStackBuffer *IPStack_Buffer_CreateBuffer(int MaxBuffers)
{
	tIPStackBuffer *ret;
	
	ret = malloc( sizeof(*ret) + MaxBuffers * sizeof(ret->SubBuffers[0]) );
	ret->MaxSubBufffers = MaxBuffers;
	ret->nSubBuffers = 0;
	ret->TotalLength = 0;
	memset(ret->SubBuffers, 0, MaxBuffers * sizeof(ret->SubBuffers[0]));
	return ret;
}

void IPStack_Buffer_DestroyBuffer(tIPStackBuffer *Buffer)
{
	// TODO: Fire callbacks?
	Buffer->MaxSubBufffers = 0;
	Buffer->nSubBuffers = 0;
	free(Buffer);
}

void IPStack_Buffer_AppendSubBuffer(tIPStackBuffer *Buffer,
	size_t HeaderLen, size_t FooterLen, const void *Data,
	tIPStackBufferCb Cb, void *Arg
	)
{
	if( Buffer->nSubBuffers == Buffer->MaxSubBufffers ) {
		// Ah, oops?
		Log_Error("IPStack", "Buffer %p only had %i sub-buffers allocated, which was not enough",
			Buffer, Buffer->MaxSubBufffers);
		return ;
	}
	
	int index = Buffer->nSubBuffers++;
	Buffer->TotalLength += HeaderLen + FooterLen;
	
	Buffer->SubBuffers[index].Data = Data;
	Buffer->SubBuffers[index].PreLength = HeaderLen;
	Buffer->SubBuffers[index].PostLength = FooterLen;
}

size_t IPStack_Buffer_GetLength(tIPStackBuffer *Buffer)
{
	return Buffer->TotalLength;
}

void *IPStack_Buffer_CompactBuffer(tIPStackBuffer *Buffer, size_t *Length)
{
	void	*ret;
	ret = malloc(Buffer->TotalLength);
	if(!ret) {
		*Length = 0;
		return NULL;
	}
	
	*Length = Buffer->TotalLength;
	
	Uint8	*dest = ret;
	for( int i = Buffer->nSubBuffers; i --; )
	{
		memcpy(dest,
			Buffer->SubBuffers[i].Data,
			Buffer->SubBuffers[i].PreLength
			);
		dest += Buffer->SubBuffers[i].PreLength;
	}
	for( int i = 0; i < Buffer->nSubBuffers; i ++ )
	{
		if( Buffer->SubBuffers[i].PostLength )
		{
			memcpy(dest,
				(Uint8*)Buffer->SubBuffers[i].Data + Buffer->SubBuffers[i].PreLength,
				Buffer->SubBuffers[i].PostLength
				);
			dest += Buffer->SubBuffers[i].PreLength;
		}
	}
	return ret;
}

int IPStack_Buffer_GetBuffer(tIPStackBuffer *Buffer, int Index, size_t *Length, const void **DataPtr)
{
	if( Index == -1 )	Index = 0;

	if( Index >= Buffer->nSubBuffers*2 ) {
		return -1;
	}

	if( Index > Buffer->nSubBuffers )
	{
		// Appended buffers
		Index -= Buffer->nSubBuffers;
	
		// Bit of a hack to avoid multiple calls which return a zero length
		while( !Buffer->SubBuffers[Index].PostLength )
		{
			if( Index++ == Buffer->nSubBuffers )
				return -1;
		}

		*DataPtr = (Uint8*)Buffer->SubBuffers[Index].Data + Buffer->SubBuffers[Index].PreLength;
		*Length = Buffer->SubBuffers[Index].PostLength;

		return (Index + 1) + Buffer->nSubBuffers;
	}
	else
	{
		Index = Buffer->nSubBuffers - Index;
		// Prepended buffers
		*DataPtr = Buffer->SubBuffers[Index].Data;
		*Length = Buffer->SubBuffers[Index].PreLength;
		return Buffer->nSubBuffers - (Index - 1);
	}
}


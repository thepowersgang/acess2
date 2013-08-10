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
	tMutex	lBufferLock;

	struct _subbuffer
	{
		const void	*Data;
		size_t	PreLength;
		size_t	PostLength;
		tIPStackBufferCb	Cb;
		void	*CbArg;
		// TODO: Callbacks?
	} SubBuffers[];
};

// === CODE ===
tIPStackBuffer *IPStack_Buffer_CreateBuffer(int MaxBuffers)
{
	tIPStackBuffer *ret;
	
	ret = malloc( sizeof(*ret) + MaxBuffers * sizeof(ret->SubBuffers[0]) );
	ASSERTR(ret, NULL);
	ret->MaxSubBufffers = MaxBuffers;
	ret->nSubBuffers = 0;
	ret->TotalLength = 0;
	memset(&ret->lBufferLock, 0, sizeof(ret->lBufferLock));
	memset(ret->SubBuffers, 0, MaxBuffers * sizeof(ret->SubBuffers[0]));
	return ret;
}

void IPStack_Buffer_ClearBuffer(tIPStackBuffer *Buffer)
{
	ASSERT(Buffer);
	IPStack_Buffer_LockBuffer(Buffer);
	for( int i = 0; i < Buffer->nSubBuffers; i ++ )
	{
		if( Buffer->SubBuffers[i].Cb == NULL )
			continue ;
		Buffer->SubBuffers[i].Cb(
			Buffer->SubBuffers[i].CbArg,
			Buffer->SubBuffers[i].PreLength,
			Buffer->SubBuffers[i].PostLength,
			Buffer->SubBuffers[i].Data
			);
	}
	Buffer->nSubBuffers = 0;
	IPStack_Buffer_UnlockBuffer(Buffer);
}

void IPStack_Buffer_DestroyBuffer(tIPStackBuffer *Buffer)
{
	ASSERT(Buffer);
	IPStack_Buffer_ClearBuffer(Buffer);
	Buffer->MaxSubBufffers = 0;
	free(Buffer);
}

void IPStack_Buffer_LockBuffer(tIPStackBuffer *Buffer)
{
	ASSERT(Buffer);
	Mutex_Acquire(&Buffer->lBufferLock);
}
void IPStack_Buffer_UnlockBuffer(tIPStackBuffer *Buffer)
{
	ASSERT(Buffer);
	Mutex_Release(&Buffer->lBufferLock);
}

void IPStack_Buffer_AppendSubBuffer(tIPStackBuffer *Buffer,
	size_t HeaderLen, size_t FooterLen, const void *Data,
	tIPStackBufferCb Cb, void *Arg
	)
{
	ASSERT(Buffer);
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
	Buffer->SubBuffers[index].Cb = Cb;
	Buffer->SubBuffers[index].CbArg = Arg;
}

size_t IPStack_Buffer_GetLength(tIPStackBuffer *Buffer)
{
	ASSERT(Buffer);
	return Buffer->TotalLength;
}

size_t IPStack_Buffer_GetData(tIPStackBuffer *Buffer, void *Dest, size_t MaxBytes)
{
	ASSERT(Buffer);
	Uint8	*dest = Dest;
	size_t	rem_space = MaxBytes;
	size_t	len;
	
	for( int i = Buffer->nSubBuffers; i -- && rem_space != 0; )
	{
		len = MIN(Buffer->SubBuffers[i].PreLength, rem_space);
		memcpy(dest,
			Buffer->SubBuffers[i].Data,
			len
			);
		dest += len;
		rem_space -= len;
	}
	for( int i = 0; i < Buffer->nSubBuffers && rem_space; i ++ )
	{
		if( Buffer->SubBuffers[i].PostLength == 0  )
			continue ;
		
		len = MIN(Buffer->SubBuffers[i].PostLength, rem_space);
		memcpy(dest,
			(Uint8*)Buffer->SubBuffers[i].Data + Buffer->SubBuffers[i].PreLength,
			len
			);
		dest += len;
		rem_space -= len;
	}
	
	return MaxBytes - rem_space;
}

void *IPStack_Buffer_CompactBuffer(tIPStackBuffer *Buffer, size_t *Length)
{
	ASSERT(Buffer);
	void	*ret = malloc(Buffer->TotalLength);
	if(!ret) {
		*Length = 0;
		return NULL;
	}
	
	*Length = Buffer->TotalLength;

	IPStack_Buffer_GetData(Buffer, ret, Buffer->TotalLength);

	return ret;
}

int IPStack_Buffer_GetBuffer(tIPStackBuffer *Buffer, int Index, size_t *Length, const void **DataPtr)
{
	ASSERT(Buffer);
	if( Index == -1 )	Index = 0;

	if( Index >= Buffer->nSubBuffers*2 ) {
		return -1;
	}

	if( Index >= Buffer->nSubBuffers )
	{
		// Appended buffers
		Index -= Buffer->nSubBuffers;
	
		// Bit of a hack to avoid multiple calls which return a zero length
		while( !Buffer->SubBuffers[Index].PostLength )
		{
			if( Index++ == Buffer->nSubBuffers )
				return -1;
		}

		if( DataPtr )
			*DataPtr = (Uint8*)Buffer->SubBuffers[Index].Data + Buffer->SubBuffers[Index].PreLength;
		if( Length )
			*Length = Buffer->SubBuffers[Index].PostLength;

		return (Index + 1) + Buffer->nSubBuffers;
	}
	else
	{
		 int	rv = Index + 1;
		Index = Buffer->nSubBuffers - Index - 1;
		// Prepended buffers
		if( DataPtr )
			*DataPtr = Buffer->SubBuffers[Index].Data;
		if( Length )
			*Length = Buffer->SubBuffers[Index].PreLength;
		return rv;
	}
}


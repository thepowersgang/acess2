/*
 * Acess2 Networking Stack
 * - By John Hodge (thePowersGang)
 *
 * include/buffer.h
 * - Outbound packet buffer management
 */
#ifndef _IPSTACK__BUFFER_H_
#define _IPSTACK__BUFFER_H_

typedef struct sIPStackBuffer	tIPStackBuffer;
typedef void	(*tIPStackBufferCb)(void *Arg, size_t HeadLen, size_t FootLen, const void *Data);

/**
 * \brief Create a buffer object, with space for \a MaxSubBuffers calls to IPStack_Buffer_AppendSubBuffer
 */
extern tIPStackBuffer	*IPStack_Buffer_CreateBuffer(int MaxSubBuffers);
/**
 * \brief Destory a created buffer object
 */
extern void	IPStack_Buffer_DestroyBuffer(tIPStackBuffer *Buffer);

/**
 * \brief Append a buffer to the object
 * \param Buffer	Buffer object from IPStack_Buffer_CreateBuffer
 * \param HeadLength	Number of bytes in \a Data that should be at the beginning of the packet
 * \param FootLength	Number of bytes in \a Data that should be at the end of the packet
 * \param Data	Actual data
 * \param Cb	Unused - eventually will be called when object is destroyed
 * \param Arg	Unused - will be argument for \a Cb
 */
extern void	IPStack_Buffer_AppendSubBuffer(tIPStackBuffer *Buffer,
	size_t HeadLength, size_t FootLength, const void *Data,
	tIPStackBufferCb Cb, void *Arg
	);

/**
 * \brief Get the total length of a buffer
 */
extern size_t	IPStack_Buffer_GetLength(tIPStackBuffer *Buffer);
/**
 * \brief Get a sub-buffer from the buffer object
 * \param PrevID	Previous return value, or -1 to start
 * \return -1 when the last buffer has been returned (*Length is not valid in this case)
 * \note Used to iterate though the buffer without compacting it
 */
extern int	IPStack_Buffer_GetBuffer(tIPStackBuffer *Buffer, int PrevID, size_t *Length, const void **Data);

/**
 * \brief Compact a buffer into a single contiguous malloc'd buffer
 * \param Buffer	Input buffer object
 * \param Length	Pointer in which to store the length of the allocated buffer
 * \return malloc'd packet data
 */
extern void	*IPStack_Buffer_CompactBuffer(tIPStackBuffer *Buffer, size_t *Length);

#endif


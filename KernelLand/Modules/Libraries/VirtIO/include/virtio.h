/*
 * Acess2 Kernel Common
 * - By John Hodge (thePowersGang)
 *
 * virtio.h
 * - VirtIO Common Header
 *
 * Reference: LinuxKernel:/Documentation/virtual/virtio-spec.txt
 */
#ifndef _VIRTIO__VIRTIO_H_
#define _VIRTIO__VIRTIO_H_

typedef struct sVirtIO_Dev	tVirtIO_Dev;
typedef struct sVirtIO_Buf	tVirtIO_Buf;

typedef enum eVirtIO_DeviceClasses	tVirtIO_DeviceClass;

/**
 * Function called when a queue entry is passed to the guest
 *
 * \param Index	Index of the entry in the queue
 */
typedef	int	(*tVirtIO_QueueCallback)(tVirtIO_Dev *Dev, int Index, size_t UsedBytes, void *Handle);

#define VIRTIO_F_NOTIFY_ON_EMPTY	(1 << 24)
#define VIRTIO_F_RING_INDIRECT_DESC	(1 << 28)
#define VIRTIO_F_RING_EVENT_IDX 	(1 << 29)

enum eVirtIO_DeviceClasses
{
	VIRTIO_DEVCLASS_NETWORK,
};

// === FUNCTIONS ===
extern tVirtIO_Dev	*VirtIO_InitDev(Uint16 IOBase, Uint IRQ, Uint32 Features, int NQueues, size_t DataSize);
extern Uint32	VirtIO_GetFeatures(tVirtIO_Dev *Dev);
extern Uint32	VirtIO_GetDevConfig(tVirtIO_Dev *Dev, int Size, Uint8 Offset);
extern void	*VirtIO_GetDataPtr(tVirtIO_Dev *Dev);
extern void	VirtIO_RemoveDev(tVirtIO_Dev *Dev);
/**
 * \brief Sets the Queue Callback
 * 
 * The queue callback is called when the device writes an entry to the used ring.
 * 
 * \param NoAutoRel	Keep descriptors in ring buffer until explicitly popped and released
 */
extern int	VirtIO_SetQueueCallback(tVirtIO_Dev *Dev, int QueueID, tVirtIO_QueueCallback Callback, int NoAutoRel);
extern tVirtIO_Buf	*VirtIO_SendBuffers(tVirtIO_Dev *Dev, int QueueID, int nBufs, size_t Sizes[], const void *Ptrs[], void *Handle);
extern tVirtIO_Buf	*VirtIO_ReceiveBuffer(tVirtIO_Dev *Dev, int QueueID, size_t Size, void *Ptr, void *Handle);
extern tVirtIO_Buf	*VirtIO_PopBuffer(tVirtIO_Dev *Dev, int QueueID, size_t *Size, const void **Ptr);\
/**
 * \brief Get the next buffer in a chain
 */
extern tVirtIO_Buf	*VirtIO_GetNextBuffer(tVirtIO_Buf *Buf);
/**
 * \brief Get the registered data pointer for this buffer
 * \note This may not be what you want. Care should be taken that this function is called from the correct address space.
 */
extern const void	*VirtIO_GetBufferPtr(tVirtIO_Buf *Buf, size_t *Size);
/**
 * \brief Get the device for a buffer
 */
extern tVirtIO_Dev	*VirtIO_GetBufferDev(tVirtIO_Buf *Buf);
/**
 * \brief Release all qdescs associated with a buffer into the free pool
 */
extern void	VirtIO_ReleaseBuffer(tVirtIO_Buf *Buffer);

#endif



/*
 * Acess2 VirtIO Common Code
 * - By John Hodge (thePowersGang)
 *
 * virtio.c
 * - Core
 */
#define DEBUG	1
#define VERSION	0x100
#include <acess.h>
#include <modules.h>
#include <semaphore.h>
#include "include/virtio.h"
#include "include/virtio_hw.h"

// === TYPES ===
typedef struct sVirtIO_Queue	tVirtIO_Queue;

// === STRUCTURES ===
struct sVirtIO_Buf
{
	Uint16	Idx;
	Uint16	Queue;
	tVirtIO_Dev	*Dev;
	void	*Handle;
	const void	*BufPtr;
};

struct sVirtIO_Queue
{
	 int	Size;
	tVirtIO_QueueCallback	Callback;
	 int	NoAutoRel;
	
	volatile struct sVirtIO_RingDesc	*Entries;
	tShortSpinlock	lAvailQueue;
	volatile struct sVirtIO_AvailRing	*Avail;
	Uint16	NextUsedPop;
	Uint16	LastSeenUsed;
	volatile struct sVirtIO_UsedRing	*Used;

	tSemaphore	FreeDescsSem;
	tShortSpinlock	lFreeList;
	Uint16	FirstUnused;	

	tVirtIO_Buf	Buffers[];
};

struct sVirtIO_Dev
{
	Uint	IRQ;
	Uint16	IOBase;
	Uint16	DevCfgBase;

	void	*DataPtr;
	
	 int	nQueues;
	struct sVirtIO_Queue	*Queues[];
};

// === PROTOTYPES ===
 int	VirtIO_Install(char **Arguments);
 int	VirtIO_Cleanup(void);
void	VirtIO_IRQHandler(int IRQ, void *Ptr);

// === GLOBALS ===
MODULE_DEFINE(0, VERSION, VirtIOCommon, VirtIO_Install, VirtIO_Cleanup, NULL);

// === CODE ===
int VirtIO_Install(char **Arguments)
{
	return 0;
}

int VirtIO_Cleanup(void)
{
	Log_Warning("VirtIO", "TODO: Cleanup");
	return 1;
}

// --- API ---
// - Device management
tVirtIO_Dev *VirtIO_InitDev(Uint16 IOBase, Uint IRQ, Uint32 Features, int MaxQueues, size_t DataSize)
{
	tVirtIO_Dev	*ret;

	// Reset and init device
	outb(IOBase + VIRTIO_REG_DEVSTS, 0);
	outb(IOBase + VIRTIO_REG_DEVSTS, VIRTIO_DEVSTS_ACKNOWLEDGE);
	outb(IOBase + VIRTIO_REG_DEVSTS, VIRTIO_DEVSTS_DRIVER);

	// Negotiate Features
	Uint32	support_feat = ind(IOBase + VIRTIO_REG_DEVFEAT);
	outd(IOBase + VIRTIO_REG_GUESTFEAT, Features & support_feat);
	LOG("Features: (Dev 0x%08x, Driver 0x%08x)", support_feat, Features);

	// Create structure
	ret = malloc( offsetof(tVirtIO_Dev, Queues[MaxQueues]) + DataSize );
	ret->IRQ = IRQ;
	ret->IOBase = IOBase;
	ret->nQueues = MaxQueues;
	ret->DataPtr = &ret->Queues[MaxQueues];

	// TODO: MSI-X makes this move
	ret->DevCfgBase = IOBase + VIRTIO_REG_DEVSPEC_0;

	// Discover virtqueues
	for( int i = 0; i < MaxQueues; i ++ )
	{
		outw(IOBase + VIRTIO_REG_QUEUESELECT, i);
		size_t qsz = inw(IOBase + VIRTIO_REG_QUEUESIZE);
		LOG("Queue #%i: QSZ = %i", i, qsz);
		if( qsz == 0 ) {
			ret->Queues[i] = NULL;
			continue ;
		}
		// TODO: Assert that qsz is a power of 2
	
		tVirtIO_Queue	*queue = calloc( offsetof(tVirtIO_Queue, Buffers[qsz]), 1 );
		queue->Size = qsz;
		queue->FirstUnused = 0;

		Semaphore_Init(&queue->FreeDescsSem, qsz, qsz, "VirtIO", "FreeDescs");

		// Allocate virtqueue spaces
		size_t	sz1 = qsz*16 + offsetof(struct sVirtIO_AvailRing, Ring[qsz])+2;
		size_t	sz2 = offsetof(struct sVirtIO_UsedRing, Ring[qsz]) + 2;
		sz1 = (sz1 + PAGE_SIZE-1) & ~(PAGE_SIZE-1);
		LOG(" sz{1,2} = 0x%x,0x%x", sz1, sz2);
		queue->Entries = MM_AllocDMA( (sz1+sz2+0xFFF)>>12, 32+12, NULL );
		queue->Avail = (void*)(queue->Entries + qsz);
		queue->Used = (void*)((char*)queue->Entries + sz1);
		
		// Clear and prepare unused list
		memset((void*)queue->Entries, 0, sz1 + sz2);
		for( int j = 0; j < qsz; j ++ )
		{
			queue->Entries[j].Flags = 1;
			queue->Entries[j].Next = j+1;
			
			queue->Buffers[j].Idx = j;
			queue->Buffers[j].Queue = i;
			queue->Buffers[j].Dev = ret;
		}
		queue->Entries[qsz-1].Flags = 0;

		ret->Queues[i] = queue;

		Uint32	queueaddr = MM_GetPhysAddr(queue->Entries) / 4096;
		LOG(" Phys %P", MM_GetPhysAddr(queue->Entries));
		outd(IOBase + VIRTIO_REG_QUEUEADDR, queueaddr);
		ASSERTC(queueaddr, ==, ind(IOBase + VIRTIO_REG_QUEUEADDR));
	}

	// Register IRQ Handler
	IRQ_AddHandler(IRQ, VirtIO_IRQHandler, ret);
	Uint8 isr = inb(IOBase + VIRTIO_REG_ISRSTS);
	LOG("isr = %x", isr);
	
	// Start
	outb(IOBase + VIRTIO_REG_DEVSTS, VIRTIO_DEVSTS_DRIVER_OK);
	
	return ret;
}

Uint32 VirtIO_GetFeatures(tVirtIO_Dev *Dev)
{
	return ind(Dev->IOBase + VIRTIO_REG_GUESTFEAT);
}
Uint32 VirtIO_GetDevConfig(tVirtIO_Dev *Dev, int Size, Uint8 Offset)
{
	switch(Size)
	{
	case 8:
		return inb(Dev->DevCfgBase + Offset);
	case 16:
		return inw(Dev->DevCfgBase + Offset);
	case 32:
		return ind(Dev->DevCfgBase + Offset);
	}
	return 0;
}
void *VirtIO_GetDataPtr(tVirtIO_Dev *Dev)
{
	return Dev->DataPtr;
}
void VirtIO_RemoveDev(tVirtIO_Dev *Dev)
{
	UNIMPLEMENTED();
}

/**
 * \brief Sets the Queue Callback
 * 
 * The queue callback is called when
 * a) a read-only queue entry is retired (device writes it to the Available ring)
 * b) a write-only queue is handed to the guest (devices writes it to the used ring)
 */
int VirtIO_SetQueueCallback(tVirtIO_Dev *Dev, int QueueID, tVirtIO_QueueCallback Callback, int NoAutoRel)
{
	ASSERTCR(QueueID, <, Dev->nQueues, -1);
	
	Dev->Queues[QueueID]->Callback = Callback;
	Dev->Queues[QueueID]->NoAutoRel = NoAutoRel;
	
	if( !Callback && NoAutoRel ) {
		Log_Warning("VirtIO", "%p:%i has had callback==NULL with auto release disabled",
			Dev, QueueID);
	}
	
	return 0;
}

int VirtIO_int_AllocQueueEntry(tVirtIO_Queue *Queue)
{
	if( Semaphore_Wait(&Queue->FreeDescsSem, 1) != 1 ) {
		return -1;
	}
	
	SHORTLOCK(&Queue->lFreeList);
	int idx = Queue->FirstUnused;
	ASSERT( Queue->Entries[idx].Flags & VRING_DESC_F_NEXT );
	Queue->FirstUnused = Queue->Entries[idx].Next;
	SHORTREL(&Queue->lFreeList);
	
	return idx;
}

tVirtIO_Buf *VirtIO_int_AllocBuf(tVirtIO_Queue *Queue, const void *Ptr, size_t Size, Uint Flags, Uint16 Next)
{
	int idx = VirtIO_int_AllocQueueEntry(Queue);
	tVirtIO_Buf	*buf = &Queue->Buffers[idx];
	ASSERTC(idx, ==, buf->Idx);

	LOG("%p:%i[%i] = {%P+0x%x}",
		buf->Dev, buf->Queue, buf->Idx,
		MM_GetPhysAddr(Ptr), Size);

	Queue->Entries[idx].Addr = MM_GetPhysAddr(Ptr);
	Queue->Entries[idx].Len = Size;
	Queue->Entries[idx].Flags = Flags;
	Queue->Entries[idx].Next = Next;

	buf->Handle = NULL;
	buf->BufPtr = Ptr;
	
	return buf;
}

tVirtIO_Buf *VirtIO_int_AllocBufV(tVirtIO_Queue *Queue, const char *Ptr, size_t Size, Uint Flags, Uint16 Next)
{
	if( ((tVAddr)Ptr & (PAGE_SIZE-1)) + Size > PAGE_SIZE*2 )
	{
		Log_Error("VirtIO", ">2 page buffers are not supported");
		return NULL;
	}
	
	tVirtIO_Buf	*ret;
	
	tPAddr	phys = MM_GetPhysAddr(Ptr);
	if( phys + Size-1 != MM_GetPhysAddr( Ptr + Size-1 ) )
	{
		size_t	fp_size = PAGE_SIZE-(phys%PAGE_SIZE);
		tVirtIO_Buf *last = VirtIO_int_AllocBuf(Queue, Ptr+fp_size, Size-fp_size, Flags, Next);
		ret = VirtIO_int_AllocBuf(Queue, Ptr, fp_size, Flags|VRING_DESC_F_NEXT, last->Idx);
	}
	else
	{
		ret = VirtIO_int_AllocBuf(Queue, Ptr, Size, Flags, Next);
	}
	return ret;
}

/*
 * Append a ring descriptor to the available ring
 */
void VirtIO_int_AddAvailBuf(tVirtIO_Queue *Queue, tVirtIO_Buf *Buf)
{
	__sync_synchronize();
	SHORTLOCK(&Queue->lAvailQueue);
	Queue->Avail->Ring[ Queue->Avail->Idx & (Queue->Size-1) ] = Buf->Idx;
	Queue->Avail->Idx ++;
	SHORTREL(&Queue->lAvailQueue);
	
	// Notify
	__sync_synchronize();
	// TODO: Delay notifications
	tVirtIO_Dev	*dev = Buf->Dev;
	outw(dev->IOBase + VIRTIO_REG_QUEUENOTIFY, Buf->Queue);
	LOG("Notifying %p:%i", Buf->Dev, Buf->Queue);
}

// Send a set of RO buffers
tVirtIO_Buf *VirtIO_SendBuffers(tVirtIO_Dev *Dev, int QueueID, int nBufs, size_t Sizes[], const void *Ptrs[], void *Handle)
{
	tVirtIO_Queue	*queue = Dev->Queues[QueueID];
	tVirtIO_Buf	*prev = NULL;

	// Allocate buffers for each non-contiguious region
	// - these come from the queue's unallocated pool
	size_t	totalsize = 0;
	for( int i = nBufs; i --; )
	{
		if( prev )
			prev = VirtIO_int_AllocBufV(queue, Ptrs[i], Sizes[i], VRING_DESC_F_NEXT, prev->Idx);
		else
			prev = VirtIO_int_AllocBufV(queue, Ptrs[i], Sizes[i], 0, 0);
		totalsize += Sizes[i];
	}
	LOG("Total size 0x%x", totalsize);

	// Final buffer has the handle set to the passed handle
	// - all others get NULL
	prev->Handle = Handle;
	
	// Add first to avaliable ring
	VirtIO_int_AddAvailBuf(queue, prev);
	
	return prev;
}

// Supply a single WO buffer for the device
tVirtIO_Buf *VirtIO_ReceiveBuffer(tVirtIO_Dev *Dev, int QueueID, size_t Size, void *Ptr, void *Handle)
{
	LOG("%p:%i - Add %p+0x%x for RX", Dev, QueueID, Ptr, Size);
	tVirtIO_Queue	*queue = Dev->Queues[QueueID];
	tVirtIO_Buf	*ret = VirtIO_int_AllocBufV(queue, Ptr, Size, VRING_DESC_F_WRITE, 0);
	ret->Handle = Handle;
	
	VirtIO_int_AddAvailBuf(queue, ret);
	return ret;
}

tVirtIO_Buf *VirtIO_PopBuffer(tVirtIO_Dev *Dev, int QueueID, size_t *Size, const void **Ptr)
{
	ASSERTCR(QueueID, <, Dev->nQueues, NULL);
	tVirtIO_Queue	*queue = Dev->Queues[QueueID];
	
	// TODO: Lock
	if( queue->NextUsedPop == queue->Used->Idx )
		return NULL;
	 int	qidx = queue->NextUsedPop;
	queue->NextUsedPop ++;

	 int	idx = queue->Used->Ring[qidx].ID;
	if( Size )
		*Size = queue->Used->Ring[qidx].Len;
	if( Ptr ) {
		*Ptr = queue->Buffers[idx].BufPtr;
		ASSERTC(MM_GetPhysAddr(*Ptr), ==, queue->Entries[idx].Addr);
	}
	return &queue->Buffers[idx];
}

const void *VirtIO_GetBufferPtr(tVirtIO_Buf *Buf, size_t *Size)
{
	tVirtIO_Queue	*queue = Buf->Dev->Queues[Buf->Queue];
	if(Size)
		*Size = queue->Entries[Buf->Idx].Len;
	return Buf->BufPtr;
}
tVirtIO_Dev *VirtIO_GetBufferDev(tVirtIO_Buf *Buf)
{
	return Buf->Dev;
}

void VirtIO_int_ReleaseQDesc(tVirtIO_Queue *Queue, Uint16 Index)
{
	LOG("Release QDesc %p:%i into free pool",
		Queue, Index);
	SHORTLOCK(&Queue->lFreeList);
	Queue->Entries[Index].Next = Queue->FirstUnused;
	Queue->Entries[Index].Flags = VRING_DESC_F_NEXT;
	Queue->FirstUnused = Index;
	SHORTREL(&Queue->lFreeList);
	Semaphore_Signal(&Queue->FreeDescsSem, 1);
}

/**
 * \brief Releases all qdescs in the buffer to the free list
 */
void VirtIO_ReleaseBuffer(tVirtIO_Buf *Buffer)
{
	 int	idx = Buffer->Idx;
	tVirtIO_Queue	*queue = Buffer->Dev->Queues[Buffer->Queue];
	
	LOG("Releasing chain at %p:%i/%i",
		Buffer->Dev, Buffer->Queue, Buffer->Idx);
	
	 int	has_next;
	do {
		has_next = !!(queue->Entries[idx].Flags & VRING_DESC_F_NEXT);
		int next_idx = queue->Entries[idx].Next;
		ASSERTC(!has_next || next_idx,!=,idx);
		
		VirtIO_int_ReleaseQDesc(queue, idx);
	
		idx = next_idx;
	} while(has_next);
}

void VirtIO_int_ProcessUsedList(tVirtIO_Dev *Dev, tVirtIO_Queue *Queue, int UsedIdx)
{
	Uint16	qent = Queue->Used->Ring[UsedIdx].ID;
	size_t	len  = Queue->Used->Ring[UsedIdx].Len;
	LOG("QEnt %i (0x%x bytes) callback w/ Handle=%p",
		qent, len, Queue->Buffers[qent].Handle);
	if( Queue->Callback )
		Queue->Callback(Dev, qent, len, Queue->Buffers[qent].Handle);
	
	if( !Queue->NoAutoRel )
	{
		// Return the buffer to the avaliable pool
		VirtIO_ReleaseBuffer(&Queue->Buffers[qent]);
		if(Queue->NextUsedPop == UsedIdx)
			Queue->NextUsedPop ++;
	}
}

void VirtIO_IRQHandler(int IRQ, void *Ptr)
{
	tVirtIO_Dev	*Dev = Ptr;
	Uint8	isr = inb(Dev->IOBase + VIRTIO_REG_ISRSTS);
	LOG("IRQ for %p - ISR = 0x%x", Dev, isr);
	
	// ISR == 0: Interrupt was not from this card
	if( isr == 0 )
		return ;
	
	// Check each queue
	for( int i = 0; i < Dev->nQueues; i ++ )
	{
		tVirtIO_Queue	*queue = Dev->Queues[i];
		// Check 'used' ring
		LOG("Queue %i Used: %i ?!= %i (Avail: %i)",
			i, queue->LastSeenUsed, queue->Used->Idx, queue->Avail->Idx);
		while( queue->LastSeenUsed != queue->Used->Idx )
		{
			int idx = queue->LastSeenUsed;
			queue->LastSeenUsed ++;
			VirtIO_int_ProcessUsedList(Dev, queue, idx);
		}
	}
}


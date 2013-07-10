/*
 * Acess2 VirtIO Common
 * - By John Hodge (thePowersGang)
 *
 * virtio_hw.h
 * - VirtIO Hardware Header
 *
 * Reference: LinuxKernel:/Documentation/virtual/virtio-spec.txt
 */
#ifndef _VIRTIO__VIRTIO_HW_H_
#define _VIRTIO__VIRTIO_HW_H_

//#if _MODULE_NAME_ != "VirtIOCommon"
//#error "This header is a VirtIO internal"
//#endif

#if 0
typedef struct sVirtIO_Header	tVirtIO_Header;

struct sVirtIO_Header
{
	volatile Uint32	DeviceFeatures;	// R
	volatile Uint32	GuestFeatures;	// RW
	Uint32	QueueAddress;	// RW
	volatile Uint16	QueueSize;	// R
	volatile Uint16	QueueSelect;	// RW
	volatile Uint16	QueueNotify;	// RW
	volatile Uint8	DeviceStatus;	// RW
	volatile Uint8	ISRStatus;	// R
};
#endif
enum eVirtIO_IOAddrs
{
	VIRTIO_REG_DEVFEAT	= 0x00,	// R
	VIRTIO_REG_GUESTFEAT	= 0x04,	// RW
	VIRTIO_REG_QUEUEADDR	= 0x08,	// RW
	VIRTIO_REG_QUEUESIZE	= 0x0C,	// R
	VIRTIO_REG_QUEUESELECT	= 0x0E,	// RW
	VIRTIO_REG_QUEUENOTIFY	= 0x10,	// RW
	VIRTIO_REG_DEVSTS	= 0x12,	// RW
	VIRTIO_REG_ISRSTS	= 0x13,	// R

	VIRTIO_REG_DEVSPEC_0	= 0x14,	

	VIRTIO_REG_MSIX_CONFIGV	= 0x14,	// RW
	VIRTIO_REG_MSIX_QUEUEV	= 0x16,	// RW
	
	VIRTIO_REG_DEVSPEC_1	= 0x18,
};

enum eVirtIO_DeviceStatuses
{
	VIRTIO_DEVSTS_RESET       = 0x00,	// Reset device
	VIRTIO_DEVSTS_ACKNOWLEDGE = 0x01,	// Acknowledged device
	VIRTIO_DEVSTS_DRIVER      = 0x02,	// Driver avaliable
	VIRTIO_DEVSTS_DRIVER_OK   = 0x04,	// Driver initialised
	VIRTIO_DEVSTS_FAILED      = 0x80,	// Something went wrong
};

enum eVirtIO_ISRBits
{
	VIRTIO_ISR_QUEUE	= 0x01,
};

// VirtIO Ring Structure
// +0   	: Ring descriptors (given by tVirtIO_Header::QueueSize)
// +QS*16	: Avaliable ring
// +PAD[4096]	: Used ring

#define VRING_AVAIL_F_NO_INTERRUPT	0x1

struct sVirtIO_AvailRing
{
	// [0]: Disable IRQ on descriptor use
	Uint16	Flags;
	Uint16	Idx;
	Uint16	Ring[];	// tVirtIO_Header::QueueSize entries
	//Uint16	UsedEvent;
};

struct sVirtIO_UsedRing
{
	// [0]: Do not notify when descriptors added
	Uint16	Flags;
	Uint16	Idx;
	struct {
		Uint32	ID;
		Uint32	Len;
	} Ring[];
	// Uint16	AvailEvent;
};

#define VRING_DESC_F_NEXT	0x1
#define VRING_DESC_F_WRITE	0x2
#define VRING_DESC_F_INDIRECT	0x4

struct sVirtIO_RingDesc
{
	Uint64	Addr;
	Uint32	Len;
	// [0]: Continue using the `Next` field
	// [1]: Write-only (insead of Read-only)
	// [2]: Indirect buffer (list of buffer descriptors)
	Uint16	Flags;
	Uint16	Next;	// Index
};

#endif


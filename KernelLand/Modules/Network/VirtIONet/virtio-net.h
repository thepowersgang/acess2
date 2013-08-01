/*
 * Acess2 VirtIO Network Driver
 * - By John Hodge (thePowersGang)
 *
 * virtio-net.h
 * - Hardware definitions
 */
#ifndef _VIRTIONET__VIRTIO_NET_H_
#define _VIRTIONET__VIRTIO_NET_H_

typedef struct sVirtIONet_PktHdr	tVirtIONet_PktHdr;

enum eVirtIO_FeatureBits
{
	VIRTIO_NET_F_CSUM	= (1 << 0),	// Checksum offloading
	VIRTIO_NET_F_GUEST_CSUM	= (1 << 1),	// ??? "Guest handles packets with partial checksum"
	VIRTIO_NET_F_MAC	= (1 << 5),	// Device has given MAC address
	// TCP Segmentation Offloading / UDP Fragmentation Offloading
	VIRTIO_NET_F_GUEST_TSO4	= (1 << 7),	// Guest can receive TSOv4
	VIRTIO_NET_F_GUEST_TSO6	= 1 << 8,	// Guest can receive TSOv6
	VIRTIO_NET_F_GUEST_TSOE	= 1 << 9,	// Guest can receive TSO with ECN (Explicit Congestion Notifcation)
	VIRTIO_NET_F_GUEST_UFO	= 1 << 10,	// Guest can recieve UFO
	VIRTIO_NET_F_HOST_TSO4	= 1 << 11,	// Device can receive TSOv4
	VIRTIO_NET_F_HOST_TSO6	= 1 << 12,	// Device can receive TSOv6
	VIRTIO_NET_F_HOST_TSOE	= 1 << 13,	// Device can receive TSO with ECN
	VIRTIO_NET_F_HOST_UFO	= 1 << 14,	// Device can recieve UFO
	
	VIRTIO_NET_F_MRG_RXBUF	= 1 << 15,	// Guest can merge recieve buffers
	VIRTIO_NET_F_STATUS	= 1 << 16,	// Configuration status field is avaliable
	// Control Channel
	VIRTIO_NET_F_CTRL_VQ	= 1 << 17,	// Control VQ is avaliable
	VIRTIO_NET_F_CTRL_RX	= 1 << 18,	// Control VQ RX mode is supported
	VIRTIO_NET_F_CTRL_VLAN	= 1 << 19,	// Control channel VLAN filtering
	VIRTIO_NET_F_GUEST_ANNOUNCE	= 1 << 21,	// "Guest can send gratuious packets"
};

#define VIRTIO_NET_S_LINK_UP	1
#define VIRTIO_NET_S_ANNOUNCE	2

struct sVirtIONet_Cfg
{
	Uint8	MACAddr[6];	// only valid if VIRTIO_NET_F_MAC
	Uint16	Status; 	// only valid if VIRTIO_NET_F_STATUS
};

enum eVirtIONet_Queues
{
	VIRTIONET_QUEUE_RX,
	VIRTIONET_QUEUE_TX,
	VIRTIONET_QUEUE_CTRL,	// only valid if VIRTIO_NET_F_CTRL_VQ
};

#define VIRTIO_NET_HDR_F_NEEDS_CSUM	1	// Checksum needs to be performed
enum eVirtIONet_GSOTypes
{
	VIRTIO_NET_HDR_GSO_NONE,
	VIRTIO_NET_HDR_GSO_TCPV4,
	VIRTIO_NET_HDR_GSO_UDP,
	VIRTIO_NET_HDR_GSO_TCPV6,
	VIRTIO_NET_HDR_GSO_ECN = 0x80
};

struct sVirtIONet_PktHdr
{
	Uint8	Flags;
	Uint8	GSOType;
	Uint16	HeaderLen;
	Uint16	GSOSize;
	Uint16	CSumStart;
	Uint16	CSumOffset;	// Offset from CSumStart
	
	Uint16	NumBuffers;	// Only if VIRTIO_NET_F_MRG_RXBUF
};

struct sVirtIONet_CmdHdr
{
	Uint8	Class;	// Command class (RX, MAC, VLAN, Announce)
	Uint8	Command;	// Actual command (RxPromisc,RxAllMulti,,MACSet,,VLANAdd,VLANDel)
	Uint8	Data[];
	// Uint8	Ack;
};

#endif


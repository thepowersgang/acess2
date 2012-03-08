/*
 * Acess2 RealTek 8169 Gigabit Network Controller Driver
 * - By John Hodge (thePowersGang)
 *
 * rtl8169.h
 * - Driver header
 */
#ifndef _RTL8169_H_
#define _RTL8169_H_

#define TXD_FLAG_OWN	0x8000	// Driver/Controller flag
#define TXD_FLAG_EOR	0x4000	// End of ring
#define TXD_FLAG_FS	0x2000	// First segment of a packet
#define TXD_FLAG_LS	0x1000	// Last segment of a packet
#define TXD_FLAG_LGSEN	0x0800	// Larget send
#define TXD_FLAG_IPCS	0x0004	// Offload IP checksum (only if LGSEN=0)
#define TXD_FLAG_UDPCS	0x0002	// Offload UDP checksum (only if LGSEN=0)
#define TXD_FLAG_TCPCS	0x0001	// Offload TCP checksum (only if LGSEN=0)

typedef struct s8169_Desc	t8169_Desc;

struct s8169_Desc
{
	Uint16	FrameLength;
	Uint16	Flags;
	Uint16	VLANTag;	// Encoded VIDL(8):PRIO(3):CFI:VIDH(4)
	Uint16	VLANFlags;
	Uint32	AddressLow;
	Uint32	AddressHigh;
} PACKED;	// sizeof() = 16 bytes

#endif


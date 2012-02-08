/*
 * Acess2 OHCI Driver
 * - By John Hodge (thePowersGang)
 *
 * ohci.h
 * - Core Header
 */
#ifndef _OHCI_H_
#define _OHCI_H_

struct sEndpointDesc
{
	//  0: 6 = Address
	//  7:10 = Endpoint Num
	// 11:12 = Direction (TD, OUT, IN, TD)
	// 13    = Speed (Full, Low)
	// 14    = Skip entry
	// 15    = Format (Others, Isochronous)
	// 16:26 = Max Packet Size
	// 27:31 = AVAIL
	Uint32	Flags;
	//  0: 3 = AVAIL
	//  4:31 = TailP
	Uint32	TailP;	// Last TD in queue
	//  0    = Halted (Queue stopped due to error)
	//  1    = Data toggle carry
	//  2: 3 = ZERO
	//  4:31 = HeadP
	Uint32	HeadP;	// First TD in queue
	//  0: 3 = AVAIL
	//  4:31 = NextED
	Uint32	NextED;	// Next endpoint descriptor
};

struct sGeneralTD
{
	//  0:17 = AVAIL
	// 18    = Buffer Rounding (Allow an undersized packet)
	// 19:20 = Direction (SETUP, OUT, IN, Resvd)
	// 21:23 = Delay Interrupt (Frame count, 7 = no int)
	// 24:25 = Data Toggle (ToggleCarry, ToggleCarry, 0, 1)
	// 26:27 = Error Count
	// 28:31 = Condition Code
	Uint32	Flags;
	
	// Base address of packet (or current when being read)
	Uint32	CBP;
	
	Uint32	NextTD;
	
	// Address of final byte in buffer
	Uint32	BE;
};

struct sIsochronousTD
{
	//  0:15 = Starting Frame
	// 16:20 = AVAIL
	// 21:23 = Delay Interrupt
	// 24:26 = Frame Count - 1 (1, 2, 3, 4, 5, 6, 7, 8)
	// 27    = AVAIL
	// 28:31 = Condition Code
	Uint32	Flags;
	
	//  0:11 = AVAIL
	// 12:31 = Page number of first byte in buffer
	Uint32	BP0;	// Buffer Page 0
	
	Uint32	NextTD;
	
	// Address of last byte in buffer
	Uint32	BufferEnd;
	
	//  0:11 = Page Offset
	// 12    = Page selector (BufferPage0, BufferEnd)
	// 13:15 = Unused?
	Uint16	Offsets[8];
};

#endif


/*
 * Acess2 VIA Rhine II Driver (VT6102)
 * - By John Hodge (thePowersGang)
 */
#ifndef _VIARHINEII__RHINE2_HW_H_
#define _VIARHINEII__RHINE2_HW_H_

enum eRegs
{
	REG_PAR0, REG_PAR1,
	REG_PAR2, REG_PAR3,
	REG_PAR4, REG_PAR5,
	REG_RCR,  REG_TCR,
	REG_CR0,  REG_CR1,
	REG_rsv0, REG_rsv1,
	REG_ISR0, REG_ISR1,
	REG_IMR0, REG_IMR1,
	
	REG_MAR0, REG_MAR1,
	REG_MAR2, REG_MAR3,
	REG_MAR4, REG_MAR5,
	REG_MAR6, REG_MAR7,
	
	REG_CUR_RX_DESC,
	REG_CUR_TX_DESC = 0x1C,

	REG_GFTEST = 0x54, REG_RFTCMD,
	REG_TFTCMD, REG_GFSTATUS,
	REG_BNRY, _REG_BNRY_HI,
	REG_CURR, _REG_CURR,
	
	//TODO: More?
};

#define RCR_SEP  	(1 << 0)	// Accept error packets
#define RCR_AR  	(1 << 1)	// Accept small packets (< 64 bytes)
#define RCR_AM  	(1 << 2)	// Accept multicast packets
#define RCR_AB  	(1 << 3)	// Accept broadcast packets
#define RCR_PROM	(1 << 4)	// Accept any addressed packet
#define RCR_RRFT(v)	(((v)&7)<<5)	// Recieve FIFO threshold (64,32,128,256,512,768,1024,s&f)

#define TCR_RSVD0	(1 << 0)	// reserved
#define TCR_LB(v)	(((v)&3)<<1)	// Loopback mode (normal, internal, MII, 223/other)
#define TCR_OFSET	(1 << 3)	// Backoff algo (VIA, Standard)
#define TCR_RSVD1	(1 << 4)	// reserved
#define TCR_TRSF(v)	(((v)&7)<<5)	// Transmit FIFO threshold

// TODO: Other Regs?

struct sRXDesc
{
	Uint16	RSR;
	Uint16	Length;	// 11 bits, Bit 15 is owner bit
	Uint16	BufferSize;	// 11 bits, Bit 15 is chain bit (means the packet continues in the next desc)
	Uint16	_resvd;
	Uint32	RXBufferStart;
	Uint32	RDBranchAddress;	// ? - I'm guessing it's the next descriptor in the chain
};

#define RSR_RERR	(1 << 0)	// Receiver error
#define RSR_CRC 	(1 << 1)	// CRC Error
#define RSR_FAE 	(1 << 2)	// Frame Alignment error
#define RSR_FOV 	(1 << 3)	// FIFO Overflow
#define RSR_LONG	(1 << 4)	// Long packet
#define RSR_RUNT	(1 << 5)	// Runt packet
#define RSR_SERR	(1 << 6)	// System Error
#define RSR_BUFF	(1 << 7)	// Buffer underflow
#define RSR_EDP 	(1 << 8)	// End of Packet
#define RSR_STP 	(1 << 9)	// Start of Packet
#define RSR_CHN 	(1 << 10)	// Chain buffer
#define RSR_PHY 	(1 << 11)	// Physical address match
#define RSR_BAR 	(1 << 12)	// Broadcast packet
#define RSR_MAR 	(1 << 13)	// Multicast packet
#define RSR_RESVD	(1 << 14)	// reserved
#define RSR_RXOK	(1 << 15)	// Recieved OK

struct sTXDesc
{
	Uint32	TSR;
	Uint16	BufferSize;	// 11 bits, bit 15 is chain
	Uint8	TCR;
	Uint8	_resvd;
	Uint32	TXBufferStart;
	Uint32	TDBranchAddress;	// Bit 0: Disable interrupt
};

#define TD_TCR_CRC	(1 << 0)	// Disable CRC generation
#define TD_TCR_STP	(1 << 5)	// First descriptor in packet
#define TD_TCR_EDP	(1 << 6)	// Last descriptor in packet
#define TD_TCR_IC	(1 << 7)	// Interrupt when transmitted

#endif


/*
 * UDI Ne2000 NIC Driver
 * By John Hodge (thePowersGang)
 *
 * ne2000_hw.h
 * - Hardware Definitions
 */
#ifndef _NE2000_HW_H_
#define _NE2000_HW_H_

#define	NE2K_MEM_START  	0x40
#define	NE2K_MEM_END		0xC0
#define NE2K_RX_FIRST_PG	(NE2K_MEM_START)
#define NE2K_RX_LAST_PG		(NE2K_MEM_START+NE2K_RX_BUF_SIZE-1)
#define	NE2K_RX_BUF_SIZE	0x40
#define NE2K_TX_FIRST_PG	(NE2K_MEM_START+NE2K_RX_BUF_SIZE)
#define NE2K_TX_LAST_PG		(NR2K_MEM_END)
#define	NE2K_TX_BUF_SIZE	0x40
#define	NE2K_MAX_PACKET_QUEUE	10

enum eNe2k_Regs
{
	NE2K_REG_CMD,
	NE2K_REG_CLDA0,		//!< Current Local DMA Address 0
	NE2K_REG_CLDA1,		//!< Current Local DMA Address 1
	NE2K_REG_BNRY,		//!< Boundary Pointer (for ringbuffer)
	NE2K_REG_TSR,		//!< Transmit Status Register
	NE2K_REG_NCR,		//!< collisions counter
	NE2K_REG_FIFO,		//!< (for what purpose ??)
	NE2K_REG_ISR,		//!< Interrupt Status Register
	NE2K_REG_CRDA0,		//!< Current Remote DMA Address 0
	NE2K_REG_CRDA1,		//!< Current Remote DMA Address 1
	// 10: RBCR0
	// 11: RBCR1
	NE2K_REG_RSR = 12	//!< Receive Status Register
};

#define NE2K_REG_PSTART	1	//!< page start (init only)
#define NE2K_REG_PSTOP	2	//!< page stop  (init only)
// 3: BNRY
#define NE2K_REG_TPSR	4	//!< transmit page start address
#define NE2K_REG_TBCR0	5	//!< transmit byte count (low)
#define NE2K_REG_TBCR1	6	//!< transmit byte count (high)
// 7: ISR
#define NE2K_REG_RSAR0	8	//!< remote start address (lo)
#define NE2K_REG_RSAR1	9	//!< remote start address (hi)
#define NE2K_REG_RBCR0	10	//!< remote byte count (lo)
#define NE2K_REG_RBCR1	11	//!< remote byte count (hi)
#define NE2K_REG_RCR	12	//!< receive config register
#define NE2K_REG_TCR	13	//!< transmit config register
#define NE2K_REG_DCR	14	//!< data config register    (init)
#define NE2K_REG_IMR	15	//!< interrupt mask register (init)
// Page 1
#define NE2K_REG_CURR	7

// Any page
#define NE2K_REG_MEM	0x10
#define NE2K_REG_RESET	0x1F

#endif


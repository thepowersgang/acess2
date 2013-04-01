
#ifndef _PCNETFAST3__HW_H_
#define _PCNETFAST3__HW_H_

typedef struct sInitBlock32	tInitBlock32;
typedef struct sTxDesc_3	tTxDesc_3;
typedef struct sRxDesc_3	tRxDesc_3;

struct sInitBlock32
{
	Uint32	Mode;	// [0:15]: MODE, [20:23] RLen, [28:31] TLen
	Uint32	PhysAddr0;	// MAC Address
	Uint32	PhysAddr1;	// [0:15] MAC Addres
	Uint32	LAdrF0;	// Logical Address Filter (CRC on multicast MAC, top 6 bits index bitmap)
	Uint32	LAdrF1;
	Uint32	RDRA;	// Rx Descriptor Ring Address
	Uint32	TDRA;	// Tx Descriptor Ring Address
};

// SWSTYLE=3: 32-bit allowing burst mode
#define RXDESC_FLG_OWN	(1<<31)
#define RXDESC_FLG_ENP	(1<<24)
#define RXDESC_FLG_STP	(1<<25)
struct sRxDesc_3
{
	Uint32	Count;	// [0:11] Message Byte Count
	Uint32	Flags;	// [0:11] BCNT, [23] BPE, ENP, STP, BUFF, CRC, OFLO, FRAM, ERR, OWN
	Uint32	Buffer;
	Uint32	_avail;	// 
};

#define TXDESC_FLG1_OWN	(1 << 31)
#define TXDESC_FLG1_ADDFCS	(1<<29)
#define TXDESC_FLG1_STP	(1 << 25)
#define TXDESC_FLG1_ENP	(1 << 24)
struct sTxDesc_3
{
	Uint32	Flags0;	// Status mostly
	Uint32	Flags1;	// [0:11] BCNT
	Uint32	Buffer;
	Uint32	_avail;
};

enum eRegs
{
	REG_APROM0	= 0x00,
	REG_APROM4	= 0x04,
	REG_APROM8	= 0x08,
	REG_APROMC	= 0x0C,
	REG_RDP 	= 0x10,	// 16 bit
	REG_RAP 	= 0x14,	// 8-bit
	REG_RESET	= 0x18,	// 16-bit
	REG_BDP 	= 0x1C,	// 16-bit
};

#define CSR_STATUS_INIT	(1<< 0)
#define CSR_STATUS_STRT	(1<< 1)
#define CSR_STATUS_IENA	(1<< 6)
#define CSR_STATUS_INTR	(1<< 7)
#define CSR_STATUS_IDON	(1<< 8)
#define CSR_STATUS_TINT	(1<< 9)
#define CSR_STATUS_RINT	(1<<10)

enum eCSR_Regs
{
	CSR_STATUS, 	// CSR0 - Am79C973/Am79C975 Controller Status
	CSR_IBA0,   	// CSR1 - Initialization Block Address[15:0]
	CSR_IBA1,   	// CSR2 - Initialization Block Address[31:16]
	CSR_INTMASK,	// CSR3 - Interrupt Masks and Deferral Control

	CSR_LAF0 = 8,	// CSR8  - Logical Address Filter[15:0]	
	CSR_LAF1 = 9,	// CSR9  - Logical Address Filter[31:16]
	CSR_LAF2 = 10,	// CSR10 - Logical Address Filter[47:32]
	CSR_LAF3 = 11,	// CSR11 - Logical Address Filter[63:48]
	CSR_MAC0 = 12,	// CSR12 - Physical Address[15:0]
	CSR_MAC1 = 13,	// CSR13 - Physical Address[31:16]
	CSR_MAC2 = 14,	// CSR14 - Physical Address[47:32]
	CSR_MODE = 15,	// CSR15 - Mode
	
	CSR_RXBASE0 = 24,	// CSR24 - Base Address of Receive Ring Lower
	CSR_RXBASE1 = 25,	// CSR25 - Base Address of Receive Ring Upper
	CSR_TXBASE0 = 30,	// CSR26 - Base Address of Transmit Ring Lower
	CSR_TXBASE1 = 31,	// CSR27 - Base Address of Transmit Ring Upper
	
	CSR_RXLENGTH = 76,	// CSR76 - Receive Ring Length
	CSR_TXLENGTH = 78,	// CSR78 - Transmit Ring Length
};

enum eBCR_Regs
{
	BCR_SWSTYLE = 20,
	BCR_PHYCS = 32,	// BCR32 - Internal PHY Control and Status
	BCR_PHYADDR,	// BCR33 - Internal PHY Address
	BCR_PHYMGMT,	// BCR34 - Internal PHY Management Data
};

#endif


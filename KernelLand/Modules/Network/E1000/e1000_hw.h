/*
 * Acess2 E1000 Network Driver
 * - By John Hodge (thePowersGang)
 *
 * e1000_hw.h
 * - Hardware Definitions
 */
#ifndef _E1000_HW_H_
#define _E1000_HW_H_

typedef struct sRXDesc	tRXDesc;
typedef struct sTXDesc	tTXDesc;

#define RXD_STS_PIF	(1<<7)	// Passed inexact filter (multicast, probably)
#define RXD_STS_IPCS	(1<<6)	// IP Checksum was calculated
#define RXD_STS_TCPCS	(1<<5)	// TCP Checksum was calculated
#define RXD_STS_RSV	(1<<4)	// reserved
#define RXD_STS_VP	(1<<3)	// Packet was 802.1q tagged
#define RXD_STS_IXSM	(1<<2)	// Ignore IPCS/TCPS
#define RXD_STS_EOP	(1<<1)	// Last descriptor in apcket
#define RXD_STS_DD	(1<<0)	// Descriptor Done, buffer data is now valid

#define RXD_ERR_RXE	(1<<7)	// RX Error
#define RXD_ERR_IPE	(1<<6)	// IP Checksum Error
#define RXD_ERR_TCPE	(1<<5)	// TCP/UDP Checksum Error
#define RXD_ERR_CXE	(1<<4)	// Carrier Extension Error (GMII cards [82544GC/EI] only)
#define RXD_ERR_SEQ	(1<<2)	// Sequence Error (aka Framing Error)
#define RXD_ERR_SE	(1<<1)	// Symbol error (TBI mode)
#define RXD_ERR_CE	(1<<0)	// CRC Error / Alignmnet Error

struct sRXDesc
{
	Uint64	Buffer;
	Uint16	Length;
	Uint16	Checksum;
	Uint8	Status;
	Uint8	Errors;
	Uint16	Special;
} PACKED;

#define TXD_CMD_IDE	(1<<7)	// Interrupt delay enable
#define TXD_CMD_VLE	(1<<6)	// VLAN Packet enable
#define TXD_CMD_DEXT	(1<<5)	// Use extended descriptor format (TODO)
#define TXD_CMD_RPS	(1<<4)	// GC/EI Report Packet Set
#define TXD_CMD_RS	(1<<3)	// Report Status
#define TXD_CMD_IC	(1<<2)	// Insert checksum at indicated offset
#define TXD_CMD_IFCS	(1<<1)	// Insert frame check sum
#define TXD_CMD_EOP	(1<<0)	// End of packet

#define TXD_STS_TU	(1<<3)	// [GC/EI] Transmit Underrun
#define TXD_STS_LC	(1<<2)	// Late collision
#define TXD_STS_EC	(1<<1)	// Excess collisions
#define TXD_STS_DD	(1<<0)	// Descriptor Done

struct sTXDesc
{
	Uint64	Buffer;
	Uint16	Length;
	Uint8	CSO;	// TCP Checksum offset
	Uint8	CMD;
	Uint8	Status;
	Uint8	CSS;	// TCP Checksum start
	Uint16	Special;
} PACKED;

#define TXCD_CMD_IDE	(1<<(24+7))	// Interrupt Delay
#define TXCD_CMD_DEXT	(1<<(24+5))	// Descriptor Extension (Must be one)
#define TXCD_CMD_RS	(1<<(24+3))	// Report Status
#define TXCD_CMD_TSE	(1<<(24+2))	// TCP Segmentation Enable
#define TXCD_CMD_IP	(1<<(24+1))	// IP version (1=IPv4, 0=IPv6)
#define TXCD_CMD_TCP	(1<<(24+0))	// Packet Type (1=TCP, 0=Other)

#define TXCD_STS_DD	(1<<0)	// Descriptor Done

struct sTXCtxDesc
{
	Uint8	IPCSS;	// IP Checksum Start
	Uint8	IPCSO;	// IP Checksum Offset
	Uint16	IPCSE;	// IP Checksum Ending (last byte)
	Uint8	TUCSS;	// TCP/UDP Checksum Start
	Uint8	TUCSO;	// TCP/UDP Checksum Offset
	Uint16	TUCSE;	// TCP/UDP Checksum Ending
	Uint32	CmdLen;	// [0:19] Length, [20:23] DTYP (0), [24:31] TUCMD
	Uint8	Status;
	Uint8	HdrLen;	// Header length
	Uint16	MSS;	// Maximum segment size
} PACKED;

#define TXDD_CMD_IDE	(1<<(24+7))	// Interrupt Delay
#define TXDD_CMD_VLE	(1<<(24+6))	// VLAN Enable
#define TXDD_CMD_DEXT	(1<<(24+5))	// Descriptor Extension
#define TXDD_CMD_RPS	(1<<(24+4))	// [GC/EI] Report Packet Sent
#define TXDD_CMD_RS	(1<<(24+3))	// Report Status
#define TXDD_CMD_TSE	(1<<(24+2))	// TCP Segmentation Enable
#define TXDD_CMD_IFCS	(1<<(24+1))	// Insert FCS
#define TXDD_CMD_EOP	(1<<(24+0))	// End of packet

#define TXDD_STS_TU	(1<<3)	// [GC/EI] Transmit Underrun
#define TXDD_STS_LC	(1<<2)	// Late collision
#define TXDD_STS_EC	(1<<1)	// Excess collisions
#define TXDD_STS_DD	(1<<0)	// Descriptor Done

#define TXDD_POPTS_TXSM	(1<<1)	// Insert TCP/UDP Checksum
#define TXDD_POPTS_IXSM	(1<<0)	// Insert IP Checksum

struct sTXDataDesc
{
	Uint64	Buffer;
	Uint32	CmdLen;	// [0:19] Length, [20:23] DTYP (1), [24:31] DCMD
	Uint8	Status;
	Uint8	POpts;	// Packet option field
	Uint16	Special;
};

#define REG32(card,ofs)	(((volatile Uint32*)card->MMIOBase)[ofs/4])
#define REG64(card,ofs)	(((volatile Uint64*)card->MMIOBase)[ofs/8])

enum eRegs
{
	REG_CTRL	= 0x000,
	REG_STATUS	= 0x008,
	REG_EECD	= 0x010,
	REG_EERD	= 0x014,
	REG_CTRL_EXT	= 0x018,
	REG_FLA 	= 0x01C,
	REG_MDIC	= 0x020,
	REG_FCAL	= 0x028,	// 64-bit value
	REG_FCAH	= 0x02C,
	REG_FCT 	= 0x030,

	REG_ICR 	= 0x0C0,	// Interrupt cause read
	REG_ITR 	= 0x0C4,	// Interrupt throttling
	REG_ICS 	= 0x0C8,	// Interrupt cause set
	REG_IMS 	= 0x0D0,	// Interrupt mask set/read
	REG_IMC 	= 0x0D8,	// Interrupt mask clear

	REG_TXCW	= 0x178,	// N/A for 82540EP/EM, 82541xx and 82547GI/EI
	REG_RXCW	= 0x180,	// ^^
	REG_LEDCTL	= 0xE00,
	
	REG_RCTL	= 0x100,
	REG_RDBAL	= 0x2800,
	REG_RDLEN	= 0x2808,
	REG_RDH 	= 0x2810,
	REG_RDT 	= 0x2818,
	REG_RDTR	= 0x2820,
	REG_RXDCTL	= 0x2820,	// Receive Descriptor Control
	
	REG_TCTL	= 0x400,
	REG_TDBAL	= 0x3800,
	REG_TDLEN	= 0x3808,
	REG_TDH 	= 0x3810,
	REG_TDT 	= 0x3818,
	REG_TDIV	= 0x3820,	// Transmit Interrupt Delay Value
	REG_TXDCTL	= 0x3828,	// Transmit Descriptor Control
	
	REG_MTA0	= 0x5200,	// 128 entries
	REG_RA0 	= 0x5400,	// 16 entries of ?
	REG_VFTA0	= 0x6500,	// 128 entries
};

#define CTRL_FD 	(1 << 0)	// Full-Duplex
#define CTRL_LRST	(1 << 3)	// Link Reset
#define CTRL_ASDE	(1 << 5)	// Auto-Speed Detection Enable
#define CTRL_SLU	(1 << 6)	// Set link up
// TODO: CTRL_*
#define CTRL_SDP0_DATA	(1 << 18)	// Software Programmable IO #0 (Data)
#define CTRL_SDP1_DATA	(1 << 19)	// Software Programmable IO #1 (Data)
// TODO: CTRL_*
#define CTRL_RST	(1 << 26)	// Software reset (cleared by hw once complete)
#define CTRL_RFCE	(1 << 27)	// Receive Flow Control Enable
#define CTRL_TFCE	(1 << 28)	// Transmit Flow Control Enable
#define CTRL_VME	(1 << 30)	// VLAN Mode Enable
#define CTRL_PHY_RST	(1 << 31)	// PHY Reset (3uS)

#define ICR_TXDW	(1 << 0)	// Transmit Descriptor Written Back
#define ICR_TXQE	(1 << 1)	// Transmit Queue Empty
#define ICR_LSC 	(1 << 2)	// Link Status Change
#define ICR_RXSEQ	(1 << 3)	// Receive Sequence Error (Framing Error)
#define ICR_RXDMT0	(1 << 4)	// Receive Descriptor Minimum Threshold Reached (need more RX descs)
#define ICR_RXO 	(1 << 6)	// Receiver overrun
#define ICR_RXT0	(1 << 7)	// Receiver Timer Interrupt
#define ICR_MDAC	(1 << 9)	// MDI/O Access Complete
#define ICR_RXCFG	(1 << 10)	// Receiving /C/ ordered sets
#define ICR_PHYINT	(1 << 12)	// PHY Interrupt
#define ICR_GPI_SDP6	(1 << 13)	// GP Interrupt SDP6/SDP2
#define ICR_GPI_SDP7	(1 << 14)	// GP Interrupt SDP7/SDP3
#define ICR_TXD_LOW	(1 << 15)	// Transmit Descriptor Low Threshold hit
#define ICR_SRPD	(1 << 16)	// Small Receive Packet Detected

#define RCTL_EN 	(1 << 1)	// Receiver Enable
#define RCTL_SBP	(1 << 2)	// Store Bad Packets
#define RCTL_UPE	(1 << 3)	// Unicast Promiscuous Enabled
#define RCTL_MPE	(1 << 4)	// Multicast Promiscuous Enabled
#define RCTL_LPE	(1 << 5)	// Long Packet Reception Enable
#define RCTL_LBM	(3 << 6)	// Loopback mode
enum {
	RCTL_LBM_NONE	= 0 << 6,	// - No Loopback
	RCTL_LBM_UD1	= 1 << 6,	// - Undefined
	RCTL_LBM_UD2	= 2 << 6,	// - Undefined
	RCTL_LBM_PHY	= 3 << 6,	// - PHY or external SerDes loopback
};
#define RCTL_RDMTS	(3 << 8)	// Receive Descriptor Minimum Threshold Size
enum {
	RCTL_RDMTS_1_2	= 0 << 8,	// - 1/2 RDLEN free
	RCTL_RDMTS_1_4	= 1 << 8,	// - 1/4 RDLEN free
	RCTL_RDMTS_1_8	= 2 << 8,	// - 1/8 RDLEN free
	RCTL_RDMTS_RSVD	= 3 << 8,	// - Reserved
};
#define RCTL_MO 	(3 << 12)	// Multicast Offset
enum {
	RCTL_MO_36	= 0 << 12,	// bits [47:36] of multicast address
	RCTL_MO_35	= 1 << 12,	// bits [46:35] of multicast address
	RCTL_MO_34	= 2 << 12,	// bits [45:34] of multicast address
	RCTL_MO_32	= 3 << 12,	// bits [43:32] of multicast address
};
#define RCTL_BAM	(1 << 15)	// Broadcast Accept Mode
#define RCTL_BSIZE	(1 << 16)	// Receive Buffer Size
enum {
	RCTL_BSIZE_2048	= (0 << 16),	// - 2048 bytes
	RCTL_BSIZE_1024	= (1 << 16),	// - 1024 bytes
	RCTL_BSIZE_512	= (2 << 16),	// - 512 bytes
	RCTL_BSIZE_256	= (3 << 16),	// - 256 bytes
	
	RCTL_BSIZE_RSVD	= (0 << 16)|(1 << 25),	// - Reserved
	RCTL_BSIZE_16K	= (1 << 16)|(1 << 25),	// - 16384 bytes
	RCTL_BSIZE_8192	= (2 << 16)|(1 << 25),	// - 8192 bytes
	RCTL_BSIZE_4096	= (3 << 16)|(1 << 25),	// - 4096 bytes
};
#define RCTL_VFE	(1 << 18)	// VLAN Filter Enable
#define RCTL_CFIEN	(1 << 19)	// Canoical Form Indicator Enable
#define RCTL_CFI	(1 << 20)	// Value to check the CFI for [Valid if CFIEN]
#define RCTL_DPF	(1 << 22)	// Discard Pause Frames
#define RCTL_PMCF	(1 << 23)	// Pass MAC Control Frames
#define RCTL_BSEX	(1 << 25)	// Buffer Size Extension (multply BSIZE by 16, used in BSIZE enum)
#define RCTL_SECRC	(1 << 26)	// Strip Ethernet CRC

#define TCTL_EN 	(1 << 1)	// Transmit Enable
#define TCTL_PSP	(1 << 3)	// Pad Short Packets
#define TCTL_CT 	(255 << 4)	// Collision Threshold
#define TCTL_CT_ofs	4
#define TCTL_COLD 	(1023 << 12)	// Collision Distance
#define TCTL_COLD_ofs	12
#define TCTL_SWXOFF	(1 << 22)	// Software XOFF Transmission
#define TCTL_RTLC	(1 << 24)	// Retransmit on Late Collision
#define TCTL_NRTU	(1 << 25)	// No retransmit on underrun [82544GC/EI]

#endif


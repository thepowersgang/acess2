/*
 * Acess2 PRO/100 Driver
 * - By John Hodge (thePowersGang)
 *
 * pro100_hw.h
 * - Hardware header
 */
#ifndef _PRO100_HW_H_
#define _PRO100_HW_H_

// Want at least 5 TX Buffer slots (for TCP)
// - However, bounce buffers are annoying, so double that
#define MIN_LOCAL_TXBUFS	5*2
// - 16 bytes per TX header, plus 8 per buffer, want 16 byte alignment
// - need to round up to multiple of 2
#define NUM_LOCAL_TXBUFS	((MIN_LOCAL_TXBUFS+1)&~(2-1))

enum ePro100_Regs {
	REG_Status,
	REG_Ack,
	REG_Command,
	REG_IntMask,
	
	REG_GenPtr = 4,
	REG_Port = 8,
	
	REG_FlashCtrl = 12,
	REG_EEPROMCtrl = 14,
	
	REG_MDICtrl = 16,
	REG_RXDMACnt = 20,
};
struct sCSR
{
	Uint16	Status;
	Uint16	Command;
	
	Uint32	GenPtr;
	Uint32	Port;
	
	Uint16	FlashCtrl;
	Uint16	EEPROMCtrl;
	
	Uint32	MDICtrl;
	Uint32	RXDMACount;
};

#define STATUS_RUS_MASK	0x3C	// Receive Unit Status
#define STATUS_CUS_MASK	0xC0	// Comamnd Unit Status
#define ISR_FCP	0x01	// Flow Control Pause
#define ISR_ER	0x02	// Early Recieve
#define ISR_SWI	0x04	// Software Interrupt
#define ISR_MDI	0x08	// Management Data Interrupt
#define ISR_RNR	0x10	// Receive Not Ready
#define ISR_CNA	0x20	// Command Unit not active
#define ISR_FR	0x40	// Frame Recieved
#define ISR_CX	0x80	// Command Unit executed

#define CMD_RUC	0x0007
#define CMD_CUC	0x00F0
#define CMD_M	0x0100	// Interrupt Mask
#define CMD_SI	0x0200	// Software Interrupt

#define MDI_IE	(1 << 29)
#define MDI_RDY	(1 << 28)

#define EEPROM_CTRL_SK	0x01	// 
#define EEPROM_CTRL_CS	0x02
#define EEPROM_CTRL_DI	0x04
#define EEPROM_CTRL_DO	0x08

#define EEPROM_OP_READ	0x06

enum ePortCommands {
	PORT_SOFTWARERESET	= 0,
	PORT_SELFTEST	= 1,
	PORT_SELECTIVERESET	= 2,
};

enum eRXCommands {
	RX_CMD_NOP,
	RX_CMD_START,
	RX_CMD_RESUME,
	_RX_CMD_3,
	RX_CMD_ABORT,
	_RX_CMD_5,
	RX_CMD_ADDR_LOAD,
	RX_CMD_RESUMENR,
};
enum eCUCommands {
	CU_CMD_NOP	= 0<<4,
	CU_CMD_START	= 1<<4,
	CU_CMD_RESUME	= 2<<4,
	CU_CMD_BASE	= 6<<4,
};

enum eCommands {
	CMD_Nop,
	CMD_IAddrSetup,
	CMD_Configure,
	CMD_MulticastList,
	CMD_Tx,
};

#define CMD_IOC	(1 << 13)	// Interrupt on completion
#define CMD_Suspend	(1 << 14)	// Suspend upon completion
#define CMD_EL	(1 << 15)	// Stop upon completion

#define CU_Status_Complete	(1 << 15)

typedef struct sCommandUnit	tCommandUnit;

struct sCommandUnit
{
	Uint16	Status;
	Uint16	Command;
	Uint32	Link;	// Relative to base
} PACKED ALIGN(4);

typedef struct sTXDescriptor	tTXDescriptor;
typedef struct sTXBufDesc	tTXBufDesc;
typedef struct sTXCommand	tTXCommand;

// - Core TX Descriptor
struct sTXDescriptor
{
	tCommandUnit	CU;
	
	Uint32	TBDArrayAddr;
	Uint16	TCBBytes;
	Uint8	TXThreshold;
	Uint8	TBDCount;
} PACKED;

// - TX Buffer descriptor (pointed to by TBDArrayAddr, or following sTXDescriptor)
struct sTXBufDesc
{
	Uint32	Addr;
	Uint16	Len;
	Uint16	EndOfList;
} PACKED;

// - TX Command block (used by static allocation)
struct sTXCommand
{
	tTXDescriptor	Desc;
	tTXBufDesc	LocalBufs[NUM_LOCAL_TXBUFS];
} PACKED;

typedef struct sRXBuffer	tRXBuffer;
struct sRXBuffer
{
	// Status:
	// - [1]: ???
	tCommandUnit	CU;

	Uint32	RXBufAddr;	// Unused according to qemu source
	Uint16	Count;
	Uint16	Size;
} PACKED;

#endif


/*
 * Acess2 PRO/100 Driver
 * - By John Hodge (thePowersGang)
 *
 * pro100_hw.h
 * - Hardware header
 */
#ifndef _PRO100_HW_H_
#define _PRO100_HW_H_

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

#define STATUS_RUS_MASK	0x003C	// Receive Unit Status
#define STATUS_CUS_MASK	0x00C0	// Comamnd Unit Status
#define STATUS_FCP	0x0100	// Flow Control Pause
#define STATUS_ER	0x0200	// Early Recieve
#define STATUS_SWI	0x0400	// Software Interrupt
#define STATUS_MDI	0x0800	// Management Data Interrupt
#define STATUS_RNR	0x1000	// Receive Not Ready
#define STATUS_CNA	0x2000	// Command Unit not active
#define STATUS_FR	0x4000	// Frame Recieved
#define STATUS_CX	0x8000	// Command Unit executed

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

typedef struct sCommandUnit
{
	Uint16	Status;
	Uint16	Command;
	Uint32	Link;
};

typedef struct sRXBuffer	tRXBuffer;

struct sRXBuffer
{
	Uint16	Status;
	Uint16	Command;
	Uint32	Link;	// Base from RX base

	Uint32	RXBufAddr;	// Unused according to qemu source
	Uint16	Count;
	Uint16	Size;
} ALIGN(4);

#endif


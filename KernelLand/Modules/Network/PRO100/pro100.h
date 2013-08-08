/*
 
 */
#ifndef _PRO100_H_
#define _PRO100_H_

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

enum ePortCommands {
	PORT_SOFTWARERESET	= 0,
	PORT_SELFTEST	= 1,
	PORT_SELECTIVERESET	= 2,
};

struct sCommandBuffer
{
	Uint16	Status;
	Uint16	Command;
	Uint32	Link;
	
	tIPStackBuffer	*Buffer;
	tCommandBuffer	*Next;
} __align__(4);

#endif


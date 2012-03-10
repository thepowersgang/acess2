/*
 * AcessOS Version 1
 * USB Stack
 * - Universal Host Controller Interface
 */
#ifndef _UHCI_H_
#define _UHCI_H_

// === TYPES ===
typedef struct sUHCI_Controller	tUHCI_Controller;
typedef struct sUHCI_ExtraTDInfo	tUHCI_ExtraTDInfo;

typedef struct sUHCI_TD	tUHCI_TD;
typedef struct sUHCI_QH	tUHCI_QH;

// === STRUCTURES ===
struct sUHCI_ExtraTDInfo
{
	 int	Offset;
	tPAddr	FirstPage;
	tPAddr	SecondPage;
	
	tUSBHostCb	Callback;
	void	*CallbackPtr;
};

struct sUHCI_TD
{
	/**
	 * \brief Next Entry in list
	 * 
	 * 31:4 - Address
	 * 3 - Reserved
	 * 2 - Depth/Breadth Select
	 * 1 - QH/TD Select
	 * 0 - Terminate (Last in List)
	 */
	Uint32	Link;
	
	/**
	 * \brief Control and Status Field
	 * 
	 * 31:30 - Reserved
	 * 29 - Short Packet Detect (Input Only)
	 * 28:27 - Number of Errors Allowed
	 * 26 - Low Speed Device (Communicating with a low speed device)
	 * 25 - Isynchonious Select
	 * 24 - Interrupt on Completion (IOC)
	 * 23:16 - Status
	 *     23 - Active
	 *     22 - Stalled
	 *     21 - Data Buffer Error
	 *     20 - Babble Detected
	 *     19 - NAK Detected
	 *     18 - CRC/Timout Error
	 *     17 - Bitstuff Error
	 *     16 - Reserved
	 * 15:11 - Reserved
	 * 10:0 - Actual Length (Number of bytes transfered)
	 */
	Uint32	Control;
	
	/**
	 * \brief Packet Header
	 * 
	 * 31:21 - Maximum Length (0=1, Max 0x4FF, 0x7FF=0)
	 * 20 - Reserved
	 * 19 - Data Toggle
	 * 18:15 - Endpoint
	 * 14:8 - Device Address
	 * 7:0 - PID (Packet Identifcation) - Only 96, E1, 2D allowed
	 *
	 * 0x96 = Data IN
	 * 0xE1 = Data Out
	 * 0x2D = Setup
	 */
	Uint32	Token;
	
	/**
	 * \brief Pointer to the data to send
	 */
	Uint32	BufferPointer;

	struct
	{
		tUHCI_ExtraTDInfo	*ExtraInfo;
		char	bActive;	// Allocated
		Uint8	QueueIndex;	// QH, 0-127 are interrupt, 128 undef, 129 Control, 130 Bulk
		char	bFreePointer;	// Free \a BufferPointer once done
	} _info;
} __attribute__((aligned(16)));

struct sUHCI_QH
{
	/**
	 * \brief Next Entry in list
	 * 
	 * 31:4 - Address
	 * 3:2 - Reserved
	 * 1 - QH/TD Select
	 * 0 - Terminate (Last in List)
	 */
	Uint32	Next;

	
	/**
	 * \brief Next Entry in list
	 * 
	 * 31:4 - Address
	 * 3:2 - Reserved
	 * 1 - QH/TD Select
	 * 0 - Terminate (Last in List)
	 */
	Uint32	Child;
	
	/*
	 * \note Area for software use
	 * \brief Last TD in this list, used to add things to the end
	 */
	tUHCI_TD	*_LastItem;
} __attribute__((aligned(16)));

struct sUHCI_Controller
{
	/**
	 * \brief PCI Device ID
	 */
	Uint16	PciId;
	
	/**
	 * \brief IO Base Address
	 */
	Uint16	IOBase;
	
	/**
	 * \brief Memory Mapped-IO base address
	 */
	Uint16	*MemIOMap;

	/**
	 * \brief IRQ Number assigned to the device
	 */
	 int	IRQNum;

	/**
	 * \brief Number of the last frame to be cleaned
	 */
	 int	LastCleanedFrame;
	
	/**
	 * \brief Frame list
	 * 
	 * 31:4 - Frame Pointer
	 * 3:2 - Reserved
	 * 1 - QH/TD Selector
	 * 0 - Terminate (Empty Pointer)
	 */
	Uint32	*FrameList;
	
	/**
	 * \brief Physical Address of the Frame List
	 */
	tPAddr	PhysFrameList;

	tUSBHub	*RootHub;

	/**
	 * \brief Load in bytes on each interrupt queue
	 */
	 int	InterruptLoad[128];

	tPAddr  	PhysTDQHPage;
	struct
	{
		// 127 Interrupt Queue Heads
		// - 4ms -> 256ms range of periods
		tUHCI_QH	InterruptQHs[0];
		tUHCI_QH	InterruptQHs_256ms[64];
		tUHCI_QH	InterruptQHs_128ms[32];
		tUHCI_QH	InterruptQHs_64ms [16];
		tUHCI_QH	InterruptQHs_32ms [ 8];
		tUHCI_QH	InterruptQHs_16ms [ 4];
		tUHCI_QH	InterruptQHs_8ms  [ 2];
		tUHCI_QH	InterruptQHs_4ms  [ 1];
		tUHCI_QH	_padding;
	
		tUHCI_QH	ControlQH;
		tUHCI_QH	BulkQH;
		
		tUHCI_TD	LocalTDPool[ (4096-(128+2)*sizeof(tUHCI_QH)) / sizeof(tUHCI_TD) ];
	}	*TDQHPage;
};

// === ENUMERATIONS ===
enum eUHCI_IOPorts {
	/**
	 * \brief USB Command Register
	 * 
	 * 15:8 - Reserved
	 * 7 - Maximum Packet Size selector (1: 64 bytes, 0: 32 bytes)
	 * 6 - Configure Flag (No Hardware Effect)
	 * 5 - Software Debug (Don't think it will be needed)
	 * 4 - Force Global Resume
	 * 3 - Enter Global Suspend Mode
	 * 2 - Global Reset (Resets all devices on the bus)
	 * 1 - Host Controller Reset (Reset just the controller)
	 * 0 - Run/Stop
	 */
	USBCMD	= 0x00,
	/**
	 * \brief USB Status Register
	 * 
	 * 15:6 - Reserved
	 * 5 - HC Halted, set to 1 when USBCMD:RS is set to 0
	 * 4 - Host Controller Process Error (Errors related to the bus)
	 * 3 - Host System Error (Errors related to the OS/PCI Bus)
	 * 2 - Resume Detect (Set if a RESUME command is sent to the Controller)
	 * 1 - USB Error Interrupt
	 * 0 - USB Interrupts (Set if a transaction with the IOC bit set is completed)
	 */
	USBSTS	= 0x02,
	/**
	 * \brief USB Interrupt Enable Register
	 * 
	 * 15:4 - Reserved
	 * 3 - Short Packet Interrupt Enable
	 * 2 - Interrupt on Complete (IOC) Enable
	 * 1 - Resume Interrupt Enable
	 * 0 - Timout / CRC Error Interrupt Enable
	 */
	USBINTR	= 0x04,
	/**
	 * \brief Frame Number (Index into the Frame List)
	 * 
	 * 15:11 - Reserved
	 * 10:0 - Index (Incremented each approx 1ms)
	 */
	FRNUM	= 0x06,
	/**
	 * \brief Frame List Base Address
	 * 
	 * 31:12 - Pysical Address >> 12
	 * 11:0 - Reserved (Set to Zero)
	 */
	FLBASEADD = 0x08,	// 32-bit
	/**
	 * \brief Start-of-frame Modify Register
	 * \note 8-bits only
	 * 
	 * Sets the size of a frame
	 * Frequency = (11936+n)/12000 kHz
	 * 
	 * 7 - Reserved
	 * 6:0 -
	 */
	SOFMOD = 0x0C,	// 8bit
	/**
	 * \brief Port Status and Controll Register (Port 1)
	 * 
	 * 15:13 - Reserved
	 * 12 - Suspend
	 * 11:10 - Reserved
	 * 9 - Port Reset
	 * 8 - Low Speed Device Attached
	 * 5:4 - Line Status
	 * 3 - Port Enable/Disable Change - Used for detecting device removal
	 * 2 - Port Enable/Disable
	 * 1 - Connect Status Change
	 * 0 - Current Connect Status
	 */
	PORTSC1 = 0x10,
	/**
	 * \brief Port Status and Controll Register (Port 2)
	 * 
	 * See ::PORTSC1
	 */
	PORTSC2 = 0x12
};

#endif

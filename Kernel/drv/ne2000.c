/* Acess2
 * NE2000 Driver
 * 
 * See: ~/Sources/bochs/bochs.../iodev/ne2k.cc
 */
#include <common.h>
#include <modules.h>
#include <fs_devfs.h>

// === CONSTANTS ===
static const struct {
	Uint16	Vendor;
	Uint16	Device;
} csaCOMPAT_DEVICES[] = {
	{0x10EC, 0x8029},	// Realtek 8029
	{0x10EC, 0x8129}	// Realtek 8129
};
#define NUM_COMPAT_DEVICES	(sizeof(csaCOMPAT_DEVICES)/sizeof(csaCOMPAT_DEVICES[0]))

enum eNe2k_Page0Read {
	COMMAND = 0,	//!< the master command register
	CLDA0,		//!< Current Local DMA Address 0
	CLDA1,		//!< Current Local DMA Address 1
	BNRY,		//!< Boundary Pointer (for ringbuffer)
	TSR,		//!< Transmit Status Register
	NCR,		//!< collisions counter
	FIFO,		//!< (for what purpose ??)
	ISR,		//!< Interrupt Status Register
	CRDA0,		//!< Current Remote DMA Address 0
	CRDA1,		//!< Current Remote DMA Address 1
	RSR = 0xC	//!< Receive Status Register
};

enum eNe2k_Page0Write {
	PTART = 1,	//!< page start (init only)
	PSTOP,		//!< page stop  (init only)
	TPSR = 4,	//!< transmit page start address
	TBCR0,		//!< transmit byte count (low)
	TBCR1,		//!< transmit byte count (high)
	RSAR0 = 8,	//!< remote start address (lo)
	RSAR1,	//!< remote start address (hi)
	RBCR0,	//!< remote byte count (lo)
	RBCR1,	//!< remote byte count (hi)
	RCR,	//!< receive config register
	TCR,	//!< transmit config register
	DCR,	//!< data config register    (init)
	IMR		//!< interrupt mask register (init)
};

// === TYPES ===
typedef struct sNe2k_Card {
	Uint16	IOBase;	//!< IO Port Address from PCI
	Uint8	IRQ;	//!< IRQ Assigned from PCI
	
	 int	NextMemPage;	//!< Next Card Memory page to use
	
	char	Name[2];	// "0"
	tVFS_Node	*Node;
	char	MacAddr[6];
} tCard;

// === PROTOTYPES ===
 int	Ne2k_Install(char **Arguments);
Uint64	Ne2k_Write(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer);

// === GLOBALS ===
MODULE_DEFINE(0, 0x0032, Ne2k, Ne2k_Install, NULL, NULL);
tDevFS_Driver	gNe2k_DriverInfo = {
	NULL, "ne2k",
	{
	.NumACLs = 1,
	.ACLs = &gVFS_ACL_EveryoneRW,
	.Flags = VFS_FFLAG_DIRECTORY
	}
};
Uint16	gNe2k_BaseAddress;
 int	giNe2k_CardCount = 0;

// === CODE ===
/**
 * \fn int Ne2k_Install(char **Options)
 * \brief Installs the NE2000 Driver
 */
int Ne2k_Install(char **Options)
{
	 int	i, j, k;
	
	// --- Scan PCI Bus ---
	// Count Cards
	giNe2k_CardCount = 0;
	for( i = 0; i < NUM_COMPAT_DEVICES; i ++ )
	{
		giNe2k_CardCount += PCI_CountDevices( csaCOMPAT_DEVICES[i].Vendor, csaCOMPAT_DEVICES[i].Device, 0 );
	}
	
	// Enumerate Cards
	k = 0;
	gpNe2k_Cards = malloc( giNe2k_CardCount * sizeof(tCard) );
	memsetd(gpNe2k_Cards, 0, giNe2k_CardCount * sizeof(tCard) / 4);
	for( i = 0; i < NUM_COMPAT_DEVICES; i ++ )
	{
		count = PCI_CountDevices( csaCOMPAT_DEVICES[i].Vendor, csaCOMPAT_DEVICES[i].Device, 0 );
		for( j = 0; j < count; j ++,k ++ )
		{
			id = PCI_GetDevice( csaCOMPAT_DEVICES[i].Vendor, csaCOMPAT_DEVICES[i].Device, 0, j );
			// Create Structure
			base = PCI_AssignPort( id, 0, 0x20 );
			gpNe2k_Cards[ k ].IOBase = base;
			gpNe2k_Cards[ k ].IRQ = PCI_GetIRQ( id );
			gpNe2k_Cards[ k ].NextMemPage = 1;
			
			//Install IRQ6 Handler
			IRQ_Set(gpNe2k_Cards[ k ].IRQ, Ne2k_IRQHandler);
			
			// Reset Card
			outb( base + 0x1F, inb(base + 0x1F) );
			while( (inb( base+ISR ) & 0x80) == 0 );
			outb( base + ISR, 0x80 );
			
			// Initialise Card
			outb( base + COMMAND, 0x21 );	// No DMA and Stop
			outb( base + DCR, 0x49 );	// Set WORD mode
			outb( base + IMR, 0x00 );
			outb( base + ISR, 0xFF );
			outb( base + RCR, 0x20 );	// Reciever to Monitor
			outb( base + TCR, 0x02 );	// Transmitter OFF
			outb( base + RBCR0, 6*4 );	// Remote Byte Count
			outb( base + RBCR1, 0 );
			outb( base + RSAR0, 0 );	// Clear Source Address
			outb( base + RSAR1, 0 );
			outb( base + COMMAND, 0x0A );	// Remote Read, Start
			
			// Read MAC Address
			gpNe2k_Cards[ k ].MacAddr[0] = inb(base+0x10);	inb(base+0x10);
			gpNe2k_Cards[ k ].MacAddr[1] = inb(base+0x10);	inb(base+0x10);
			gpNe2k_Cards[ k ].MacAddr[2] = inb(base+0x10);	inb(base+0x10);
			gpNe2k_Cards[ k ].MacAddr[3] = inb(base+0x10);	inb(base+0x10);
			gpNe2k_Cards[ k ].MacAddr[4] = inb(base+0x10);	inb(base+0x10);
			gpNe2k_Cards[ k ].MacAddr[5] = inb(base+0x10);	inb(base+0x10);
			
			outb( base+PSTART, 0x60);	// Set Receive Start
			outb( base+BNRY, 0x7F);	// Set Boundary Page
			outb( base+PSTOP, 0x80);	// Set Stop Page
			outb( base+ISR, 0xFF );	// Clear all ints
			outb( base+CMD, 0x22 );	// No DMA, Start
			outb( base+IMR, 0x3F );	// Set Interupt Mask
			outb( base+RCR, 0x8F );	// Set WRAP and allow all packet matches
			outb( base+TCR, 0x00 );	// Set Normal Transmitter mode
			outb( base+TPSR, 0x40);	// Set Transmit Start
			
			// Set MAC Address
			/*
			Ne2k_WriteReg(base, MAC0, gpNe2k_Cards[ k ].MacAddr[0]);
			Ne2k_WriteReg(base, MAC1, gpNe2k_Cards[ k ].MacAddr[1]);
			Ne2k_WriteReg(base, MAC2, gpNe2k_Cards[ k ].MacAddr[2]);
			Ne2k_WriteReg(base, MAC3, gpNe2k_Cards[ k ].MacAddr[3]);
			Ne2k_WriteReg(base, MAC4, gpNe2k_Cards[ k ].MacAddr[4]);
			Ne2k_WriteReg(base, MAC5, gpNe2k_Cards[ k ].MacAddr[5]);
			*/
			
			Log("[NE2K]: Card #%i: IRQ=%i, Base=0x%x, ",
				k, gpNe2k_Cards[ k ].IRQ, gpNe2k_Cards[ k ].PortBase,
				gpNe2k_Cards[ k ].Buffer);
			Log("MAC Address %x:%x:%x:%x:%x:%x\n",
				gpNe2k_Cards[ k ].MacAddr[0], gpNe2k_Cards[ k ].MacAddr[1],
				gpNe2k_Cards[ k ].MacAddr[2], gpNe2k_Cards[ k ].MacAddr[3],
				gpNe2k_Cards[ k ].MacAddr[4], gpNe2k_Cards[ k ].MacAddr[5]
				);
			
			// Set VFS Node
			gpNe2k_Cards[ k ].Name[0] = '0'+k;
			gpNe2k_Cards[ k ].Name[1] = '\0';
			gpNe2k_Cards[ k ].Node.NumACLs = 1;
			gpNe2k_Cards[ k ].Node.ACLs = gVFS_ACL_EveryoneRW;
			gpNe2k_Cards[ k ].Node.CTime = now();
			gpNe2k_Cards[ k ].Node.Write = Ne2k_Write;
		}
	}
	
	DevFS_AddDevice( &gNe2k_DriverInfo );
	return 0;
}


/**
 * \fn Uint64 Ne2k_Write(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer)
 */
Uint64 Ne2k_Write(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer)
{
	tCard	*Card = (tCard*)Node->ImplPtr;
	
	// Sanity Check Length
	if(Length > 0xFFFF)	return 0;
	
	outb(Card->IOBase + COMMAND, 0|0x22);	// Page 0, Start, NoDMA
	// Send Size
	outb(Card->IOBase + RBCR0, Count & 0xFF);
	outb(Card->IOBase + RBCR1, Count >> 8);
	// Clear Remote DMA Flag
	outb(Card->IOBase + ISR, 0x40);	// Bit 6
	// Set up transfer
	outb(Card->IOBase + RSAR0, 0x00);	// Page Offset
	outb(Card->IOBase + RSAR1, Ne2k_int_GetWritePage(Card, Length));	// Page Offset
	return 0;
}

/**
 * \fn Uint8 Ne2k_int_GetWritePage(tCard *Card, Uint16 Length)
 */
Uint8 Ne2k_int_GetWritePage(tCard *Card, Uint16 Length)
{
	
}

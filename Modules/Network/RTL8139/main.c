/* Acess2 RTL8139 Driver
 * - By John Hodge (thePowersGang)
 * 
 * main.c - Driver Core
 */
#define	DEBUG	0
#define VERSION	((0<<8)|50)
#include <acess.h>
#include <modules.h>
#include <fs_devfs.h>
#include <drv_pci.h>
#include <tpl_drv_network.h>

// === CONSTANTS ===
enum eRTL8139_Regs
{
	MAC0, MAC1, MAC2,
	MAC3, MAC4, MAC5,
	MAR0	= 0x08,
	MAR1, MAR2, MAR3,
	MAR4, MAR5, MAR6, MAR7,
	
	RBSTART = 0x30,	//!< Recieve Buffer Start
	// ??, ??, ??, RST, RE, TE, ??, ??
	CMD 	= 0x37,
	IMR 	= 0x3C,
	ISR 	= 0x3E,
	
	RCR 	= 0x44,
	
	CONFIG1	= 0x52
};

// === TYPES ===
typedef struct sCard
{
	Uint16	IOBase;
	Uint8	IRQ;
	
	 int	NumWaitingPackets;
	
	void	*ReceiveBuffer;
	tPAddr	PhysReceiveBuffer;
	
	char	Name[2];
	tVFS_Node	Node;
	Uint8	MacAddr[6];
}	tCard;

// === PROTOTYPES ===

// === GLOBALS ===
MODULE_DEFINE(0, VERSION, RTL8139, RTL8139_Install, NULL, NULL);
 int	giRTL8139_CardCount;
tCard	gpRTL8139_Cards;

// === CODE ===
/**
 * \brief Installs the RTL8139 Driver
 */
int RTL8139_Install(char **Options)
{
	 int	id = -1;
	 int	i = 0;
	Uint16	base;
	
	giRTL8139_CardCount = PCI_CountDevices( 0x10EC, 0x8139, 0 );
	
	gpRTL8139_Cards = calloc( giRTL8139_CardCount, sizeof(tCard) );
	
	
	while( (id = PCI_GetDevice(0x10EC, 0x8139, 0, id)) != -1 )
	{
		base = PCI_AssignPort( id, 0, 0x100 );
		gpRTL8139_Cards[i].IOBase = base;
		gpRTL8139_Cards[i].IRQ = PCI_GetIRQ( id );
		
		// Install IRQ Handler
		IRQ_AddHandler(gpRTL8139_Cards[ k ].IRQ, RTL8136_IRQHandler);
		
		// Power on
		outb( base + CONFIG1, 0x00 );
		// Reset (0x10 to CMD)
		outb( base + CMD, 0x10 );
		
		gpRTL8139_Cards[i].ReceiveBuffer = MM_AllocDMA( 2, 32, &gpRTL8139_Cards[i].PhysReceiveBuffer );
		// Set up recieve buffer
		outl(base + RBSTART, (Uint32)gpRTL8139_Cards[i].PhysReceiveBuffer);
		// Set IMR to Transmit OK and Receive OK
		outw(base + IMR, 0x5);
		
		// Set recieve buffer size, buffer wrap and recieve mask
		outl(base + RCR, 0x8F);
		
		outb(base + CMD, 0x0C);	// Recive Enable and Transmit Enable
		
		gpRTL8139_Cards[ i ].MacAddr[0] = inb(base+MAC0);
		gpRTL8139_Cards[ i ].MacAddr[1] = inb(base+MAC1);
		gpRTL8139_Cards[ i ].MacAddr[2] = inb(base+MAC2);
		gpRTL8139_Cards[ i ].MacAddr[3] = inb(base+MAC3);
		gpRTL8139_Cards[ i ].MacAddr[4] = inb(base+MAC4);
		gpRTL8139_Cards[ i ].MacAddr[5] = inb(base+MAC5);
		
		// Set VFS Node
		gpRTL8139_Cards[ i ].Name[0] = '0'+i;
		gpRTL8139_Cards[ i ].Name[1] = '\0';
		gpRTL8139_Cards[ i ].Node.ImplPtr = &gpRTL8139_Cards[ i ];
		gpRTL8139_Cards[ i ].Node.NumACLs = 0;
		gpRTL8139_Cards[ i ].Node.CTime = now();
		gpRTL8139_Cards[ i ].Node.Write = RTL8139_Write;
		gpRTL8139_Cards[ i ].Node.Read = RTL8139_Read;
		gpRTL8139_Cards[ i ].Node.IOCtl = RTL8139_IOCtl;
		
		Log_Log("RTL8139", "Card %i 0x%04x %02x:%02x:%02x:%02x:%02x:%02x",
			i, base,
			gpRTL8139_Cards[ i ].MacAddr[0], gpRTL8139_Cards[ i ].MacAddr[1],
			gpRTL8139_Cards[ i ].MacAddr[2], gpRTL8139_Cards[ i ].MacAddr[3],
			gpRTL8139_Cards[ i ].MacAddr[4], gpRTL8139_Cards[ i ].MacAddr[5]
			);
		
		i ++;
	}
	return MODULE_ERR_OK;
}

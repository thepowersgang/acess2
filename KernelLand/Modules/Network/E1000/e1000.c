/*
 * Acess2 E1000 Network Driver
 * - By John Hodge (thePowersGang)
 *
 * e1000.c
 * - Intel E1000 Network Card Driver (core)
 */
#define DEBUG	1
#define	VERSION	VER2(0,1)
#include <acess.h>
#include "e1000.h"
#include <modules.h>
#include <drv_pci.h>
#include <IPStack/include/adapters_api.h>

// === PROTOTYPES ===
 int	E1000_Install(char **Arguments);
 int	E1000_Cleanup(void);
tIPStackBuffer	*E1000_WaitForPacket(void *Ptr);
 int	E1000_SendPacket(void *Ptr, tIPStackBuffer *Buffer);
void	E1000_IRQHandler(int Num, void *Ptr);

// === GLOBALS ===
MODULE_DEFINE(0, VERSION, E1000, E1000_Install, E1000_Cleanup, NULL);
tIPStack_AdapterType	gE1000_AdapterType = {
	.Name = "E1000",
	.Type = 0,	// TODO: Differentiate differnet wire protos and speeds
	.Flags = 0,	// TODO: IP checksum offloading, MAC checksum offloading etc
	.SendPacket = E1000_SendPacket,
	.WaitForPacket = E1000_WaitForPacket
	};

// === CODE ===
int E1000_Install(char **Arguments)
{
	for( int id = -1; (id = PCI_GetDevice(0x8086, 0x100E, id)) != -1; )
	{
		
	}
	return MODULE_ERR_NOTNEEDED;
}

int E1000_Cleanup(void)
{
	return 0;
}

tIPStackBuffer *E1000_WaitForPacket(void *Ptr)
{
	return NULL;
}

int E1000_SendPacket(void *Ptr, tIPStackBuffer *Buffer)
{
	return -1;
}

void E1000_IRQHandler(int Num, void *Ptr)
{
}

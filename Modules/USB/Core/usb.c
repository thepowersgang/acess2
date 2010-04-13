/*
 * Acess 2 USB Stack
 * USB Packet Control
 */
#define DEBUG	1
#include <acess.h>
#include <vfs.h>
#include <drv_pci.h>
#include "usb.h"


// === CODE ===
void USB_MakeToken(void *Buf, int PID, int Addr, int EndP)
{
	Uint8	*tok = Buf;
	 int	crc = 0;
	
	tok[0] = PID & 0xFF;
	tok[1] = (Addr & 0x7F) | ((EndP&1)<<7);
	tok[2] = ((EndP >> 1) & 0x7) | crc;
}

#if 0
void USB_SendData(int Controller, int Dev, int Endpoint, void *Data, int Length)
{
	Uint8	buf[Length+3+2/*?*/];
	
	USB_MakeToken(buf, PID_DATA0, Dev, Endpoint);
	
	switch(Controller & 0xF00)
	{
	case 1:	// UHCI
		UHCI_SendPacket(Controller & 0xFF);
		break;
	}
}
#endif

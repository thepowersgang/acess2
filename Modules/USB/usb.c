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
	 int	crc = 0;	//USB_TokenCRC();
	
	tok[0] = PID;
	tok[1] = Addr | ((EndP&1)<<7);
	tok[2] = (EndP >> 1) | crc;
}

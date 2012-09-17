/*
 * Acess2 Kernel
 * - By John Hodge (thePowersGang)
 *
 * drv/vpci.c
 * - Virtual PCI Bus
 */
#include <virtual_pci.h>

// === CODE ===
Uint32 VPCI_Read(tVPCI_Device *Dev, Uint8 Offset, Uint8 Size)
{
	Uint32	tmp_dword = 0;

	if( Size > 4 || Size == 3 || Size == 0 )
		return 0;
	if( Offset & (Size - 1) )
		return 0;
	
	switch( Offset >> 2 )
	{
	case 0:	// Vendor[0:15], Device[16:31]
		tmp_dword = (Dev->Vendor) | (Dev->Device << 16);
		break;
	case 2:	// Class Code
		tmp_dword = Dev->Class;
		break;
	// 1: Command[0:15], Status[16:31]
	// 3: Cache Line Size, Latency Timer, Header Type, BIST
	// 4-9: BARs
	// 10: Unused (Cardbus CIS Pointer)
	// 11: Subsystem Vendor ID, Subsystem ID
	// 12: Expansion ROM Address
	// 13: Capabilities[0:8], Reserved[9:31]
	// 14: Reserved
	// 15: Interrupt Line, Interrupt Pin, Min Grant, Max Latency
	default:
		tmp_dword = Dev->Read(Dev->Ptr, Offset >> 2);
		break;
	}

	tmp_dword >>= 8*(Offset & 3);
	switch(Size)
	{
	case 4:	break;
	case 2:	tmp_dword &= 0xFFFF;	break;
	case 1:	tmp_dword &= 0xFF;
	}

	return tmp_dword;
}

void VPCI_Write(tVPCI_Device *Dev, Uint8 Offset, Uint8 Size, Uint32 Data)
{
	Uint32	tmp_dword;
	if( Size > 4 || Size == 3 || Size == 0 )
		return ;
	if( Offset & (Size - 1) )
		return ;
	
	switch(Offset >> 2)
	{
	case 0:	// Vendor / Device IDs
	case 2:	// Class Code
		// READ ONLY
		return ;
	}

	tmp_dword = Dev->Read(Dev->Ptr, Offset>>2);
	switch(Size)
	{
	case 4:	tmp_dword = 0;	break;
	case 2:
		tmp_dword &= ~(0xFFFF << ((Offset&2)*16));
		Data |= 0xFFFF;
		break;
	case 1:
		tmp_dword &= ~(0xFF << ((Offset&3)*8));
		Data |= 0xFF;
		break;
	}
	tmp_dword |= Data << ((Offset&3)*8);
	Dev->Write(Dev->Ptr, Offset>>2, tmp_dword);
}

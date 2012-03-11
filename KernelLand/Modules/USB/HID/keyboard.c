/*
 * Acess2 USB Stack HID Driver
 * - By John Hodge (thePowersGang)
 *
 * keyboard.c
 * - Keyboard translation
 */
#define DEBUG	0
#include <acess.h>
#include <fs_devfs.h>
#include "hid_reports.h"
#include <Input/Keyboard/include/keyboard.h>

typedef struct sUSB_Keyboard	tUSB_Keyboard;

// === STRUCTURES ===
struct sUSB_Keyboard
{
	void	*Next;	// Is this needed? (I think main.c wants it)
	tUSB_DataCallback	DataAvail;
	
	tKeyboard *Info;
	 int	bIsBoot;
	
	// Boot keyboard hackery
	Uint8	oldmods;
	Uint8	oldkeys[6];
	
	 int	CollectionDepth;
};

// === PROTOTYPES ===
void	HID_Kb_DataAvail(tUSBInterface *Dev, int EndPt, int Length, void *Data);

tHID_ReportCallbacks	*HID_Kb_Report_Collection(tUSBInterface *Dev, tHID_ReportGlobalState *Global, tHID_ReportLocalState *Local, Uint32 Value);
void	HID_Kb_Report_EndCollection(tUSBInterface *Dev);
void	HID_Kb_Report_Input(tUSBInterface *Dev, tHID_ReportGlobalState *Global, tHID_ReportLocalState *Local, Uint32 Value);

// === GLOBALS ===
tHID_ReportCallbacks	gHID_Kb_ReportCBs = {
	.Collection = HID_Kb_Report_Collection,
	.EndCollection = HID_Kb_Report_EndCollection,
	.Input = HID_Kb_Report_Input
};

// === CODE ===
void HID_Kb_DataAvail(tUSBInterface *Dev, int EndPt, int Length, void *Data)
{
	tUSB_Keyboard	*info;
	info = USB_GetDeviceDataPtr(Dev);

	LOG("info = %p", info);
	
	if( info->bIsBoot )
	{
		Uint8	*bytes = Data;
		Uint8	modchange = bytes[0] ^ info->oldmods;
		LOG("modchange = %x", modchange);
		// Modifiers
		const int firstmod = 224;	// from qemu
		for( int i = 0; i < 8; i ++ )
		{
			if( !(modchange & (1 << i)) )	continue ;
			if( bytes[0] & (1 << i) ) {
				LOG("mod press %i", firstmod+i);
				Keyboard_HandleKey(info->Info, firstmod + i);
			}
			else {
				LOG("mod release %i", firstmod+i);
				Keyboard_HandleKey(info->Info, (1 << 31)|(firstmod + i));
			}
		}
		info->oldmods = bytes[0];
		
		// Keycodes
		// - Presses
		for( int i = 0; i < 6; i ++ )
		{
			 int	j;
			Uint8	code = bytes[2+i];
			// Check if the button was pressed before
			for( j = 0; j < 6; j ++ )
			{
				if( info->oldkeys[j] == code )
				{
					info->oldkeys[j] = 0;
					break;
				}
			}
			// Press?
			if( j == 6 ) {
				LOG("press %i", code);
				Keyboard_HandleKey(info->Info, (0 << 31)|code);
			}
		}
		// - Check for releases
		for( int i = 0; i < 6; i ++ )
		{
			if( info->oldkeys[i] ) {
				LOG("release %i", info->oldkeys[i]);
				Keyboard_HandleKey(info->Info, (1 << 31)|info->oldkeys[i]);
			}
		}
		// - Update state
		memcpy(info->oldkeys, bytes+2, 6);
	}
	else
	{
		// Oops... TODO: Support non boot keyboards
	}
}

// --- ---
tHID_ReportCallbacks *HID_Kb_Report_Collection(
	tUSBInterface *Dev, tHID_ReportGlobalState *Global, tHID_ReportLocalState *Local,
	Uint32 Value
	)
{
	return &gHID_Kb_ReportCBs;
}

void HID_Kb_Report_EndCollection(tUSBInterface *Dev)
{
}

void HID_Kb_Report_Input(tUSBInterface *Dev, tHID_ReportGlobalState *Global, tHID_ReportLocalState *Local, Uint32 Value)
{
	tUSB_Keyboard	*info;

	info = USB_GetDeviceDataPtr(Dev);
	if( !info )
	{
		info = malloc( sizeof(tUSB_Keyboard) );
		LOG("info = %p", info);
		USB_SetDeviceDataPtr(Dev, info);
		info->DataAvail = HID_Kb_DataAvail;
		info->Info = NULL;
		info->CollectionDepth = 1;
		info->bIsBoot = 1;	// TODO: Detect non-boot keyboards and parse descriptor
		Log_Warning("USB HID", "TODO: Handle non-boot keyboards!");
		info->Info = Keyboard_CreateInstance(0, "USBKeyboard");
	}
}


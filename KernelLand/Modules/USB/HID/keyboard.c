/*
 * Acess2 USB Stack HID Driver
 * - By John Hodge (thePowersGang)
 *
 * keyboard.c
 * - Keyboard translation
 */
#define DEBUG	0
#include <fs_devfs.h>
#include <Input/Keyboard/include/keyboard.h>

typedef struct sUSB_Keyboard	tUSB_Keyboard;

// === STRUCTURES ===
struct sUSB_Keyboard
{
	tKeyboard *Info;
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
	
}

// --- ---
tHID_ReportCallbacks *HID_Kb_Report_Collection(
	tUSBInterface *Dev, tHID_ReportGlobalState *Global, tHID_ReportLocalState *Local,
	Uint32 Value
	)
{
	tUSB_Keyboard	*info;
	
	info = USB_GetDeviceDataPtr(Dev);
	if( !info )
	{
		info = malloc( sizeof(tUSB_Keyboard) );
		USB_SetDeviceDataPtr(Dev, info);
		info->Keyboard = NULL;
		info->CollectionDepth = 1;
	}
	else
	{
		info->CollectionDepth ++;
	}

	return &gHID_Kb_ReportCBs;
}

void HID_Kb_Report_EndCollection(tUSBInterface *Dev)
{
	tUSB_Keyboard	*info;
	
	info = USB_GetDeviceDataPtr(Dev);
	if( !info )	return ;
	
	info->CollectionDepth --;
	if( info->CollectionDepth == 0 )
	{
		info->Keyboard = Keyboard_CreateInstance(0, "USBKeyboard");
	}
}

void HID_Kb_Report_Input(tUSBInterface *Dev, tHID_ReportGlobalState *Global, tHID_ReportLocalState *Local, Uint32 Value)
{

}


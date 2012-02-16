/*
 * Acess2 USB Stack HID Driver
 * - By John Hodge (thePowersGang)
 *
 * mouse.c
 * - USB Mouse driver
 */
#define DEBUG	0
#include <acess.h>
#include "hid_reports.h"
#include <fs_devfs.h>


// === PROTOTYES ===
tHID_ReportCallbacks	*HID_Mouse_Report_Collection(tUSBInterface *Dev, tHID_ReportGlobalState *Global, tHID_ReportLocalState *Local, Uint32 Value);
void	HID_Mouse_Report_Input(tUSBInterface *Dev, tHID_ReportGlobalState *Global, tHID_ReportLocalState *Local, Uint32 Value);
void	HID_Mouse_Report_Output(tUSBInterface *Dev, tHID_ReportGlobalState *Global, tHID_ReportLocalState *Local, Uint32 Value);
void	HID_Mouse_Report_Feature(tUSBInterface *Dev, tHID_ReportGlobalState *Global, tHID_ReportLocalState *Local, Uint32 Value);

// === GLOBALS ===
tDevFS_Driver	gHID_Mouse_DevFS = {
	.Name = "USBKeyboard",
};
tHID_ReportCallbacks	gHID_Mouse_ReportCBs = {
	.Collection = HID_Mouse_Report_Collection,
	.Input = HID_Mouse_Report_Input,
	.Output = HID_Mouse_Report_Output,
	.Feature = HID_Mouse_Report_Feature
};

// === CODE ===
tHID_ReportCallbacks *HID_Mouse_Report_Collection(
	tUSBInterface *Dev,
	tHID_ReportGlobalState *Global, tHID_ReportLocalState *Local,
	Uint32 Value
	)
{
	return &gHID_Mouse_ReportCBs;
}

void HID_Mouse_Report_Input(
	tUSBInterface *Dev,
	tHID_ReportGlobalState *Global, tHID_ReportLocalState *Local,
	Uint32 Value
	)
{
	
}

void HID_Mouse_Report_Output(
	tUSBInterface *Dev,
	tHID_ReportGlobalState *Global, tHID_ReportLocalState *Local,
	Uint32 Value
	)
{
	
}

void HID_Mouse_Report_Feature(
	tUSBInterface *Dev,
	tHID_ReportGlobalState *Global, tHID_ReportLocalState *Local,
	Uint32 Value
	)
{
	
}


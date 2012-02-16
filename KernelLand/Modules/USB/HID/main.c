/*
 * Acess2 USB Stack HID Driver
 * - By John Hodge (thePowersGang)
 *
 * main.c
 * - Driver Core
 */
#define DEBUG	1
#define VERSION	VER2(0,1)
#include <acess.h>
#include <modules.h>
#include <usb_core.h>
#include "hid.h"
#include "hid_reports.h"

// === TYPES ===
typedef struct sHID_Device	tHID_Device;

struct sHID_Device
{
	void	*Next;	// Used by sub-driver
	tUSB_DataCallback	DataAvail;
	// ... Device-specific data
};

// === PROTOTYPES ===
 int	HID_Initialise(char **Arguments);
void	HID_InterruptCallback(tUSBInterface *Dev, int EndPt, int Length, void *Data);
void	HID_DeviceConnected(tUSBInterface *Dev, void *Descriptors, size_t DescriptorsLen);
tHID_ReportCallbacks	*HID_RootCollection(tUSBInterface *Dev, tHID_ReportGlobalState *Global, tHID_ReportLocalState *Local, Uint32 Value);
void	HID_int_ParseReport(tUSBInterface *Dev, Uint8 *Data, size_t Length, tHID_ReportCallbacks *StartCBs);

static void	_AddItem(struct sHID_IntList *List, Uint32 Value);
static void	_AddItems(struct sHID_IntList *List, Uint32 First, Uint32 Last);
static void	_FreeList(struct sHID_IntList *List);

// === GLOBALS ===
MODULE_DEFINE(0, VERSION, USB_HID, HID_Initialise, NULL, "USB_Core", NULL);
tUSBDriver	gHID_USBDriver = {
	.Name = "HID",
	.Match = {.Class = {0x030000, 0xFF0000}},
	.Connected = HID_DeviceConnected,
	.MaxEndpoints = 2,
	.Endpoints = {
		{0x80, HID_InterruptCallback},
		{0, NULL}
	}
};
tHID_ReportCallbacks	gHID_RootCallbacks = {
	.Collection = HID_RootCollection
};

// === CODE ===
int HID_Initialise(char **Arguments)
{
	USB_RegisterDriver( &gHID_USBDriver );
	return 0;
}

/**
 * \brief Callback for when there's new data from the device
 * 
 * Calls the subdriver callback (stored at a fixed offset in the device data structure)
 */
void HID_InterruptCallback(tUSBInterface *Dev, int EndPt, int Length, void *Data)
{
	tHID_Device	*info;
	
	info = USB_GetDeviceDataPtr(Dev);
	if(!info) {
		Log_Error("USB HID", "Device %p doesn't have a data pointer.", Dev);
		return ;
	}
	
	info->DataAvail(Dev, EndPt, Length, Data);
}

/**
 * \brief Handle a device connection
 */
void HID_DeviceConnected(tUSBInterface *Dev, void *Descriptors, size_t DescriptorsLen)
{
	struct sDescriptor_HID	*hid_desc;
	size_t	ofs = 0;
	size_t	report_len = 0;
	
	ENTER("pDev pDescriptors iDescriptorsLen", Dev, Descriptors, DescriptorsLen);

	// --- Locate HID descriptor ---
	hid_desc = NULL;
	while(ofs + 2 <= DescriptorsLen)
	{
		hid_desc = (void*)( (char*)Descriptors + ofs );
		ofs += hid_desc->Length;
		// Sanity check length
		if( ofs > DescriptorsLen ) {
			hid_desc = NULL;
			break ;
		}
		// Check for correct type
		if( hid_desc->Type == 0x21 )
			break ;
		// On to the next one then
	}
	if( hid_desc == NULL ) {
		Log_Warning("USB_HID", "Device doesn't have a HID descritor");
		LEAVE('-');
		return ;
	}
	// - Sanity check length
	if( hid_desc->Length < sizeof(*hid_desc) + hid_desc->NumDescriptors * sizeof(hid_desc->Descriptors[0]) )
	{
		// Too small!
		Log_Warning("USB_HID", "HID Descriptor undersized (%i < %i)",
			hid_desc->Length,
			sizeof(*hid_desc) + hid_desc->NumDescriptors * sizeof(hid_desc->Descriptors[0]));
		LEAVE('-');
		return ;
	}

	// --- Dump descriptor header ---
	LOG("hid_desc = {");
	LOG("  .Length  = %i", hid_desc->Length);
	LOG("  .Type    = 0x%x", hid_desc->Type);
	LOG("  .Version = 0x%x", hid_desc->Version);
	LOG("  .NumDescriptors = %i", hid_desc->NumDescriptors);
	LOG("}");

	// --- Locate report descriptor length ---
	for( int i = 0; i < hid_desc->NumDescriptors; i ++ )
	{
		if( hid_desc->Descriptors[i].DescType == 0x22 ) {
			report_len = LittleEndian16( hid_desc->Descriptors[i].DescLen );
			break ;
		}
	}
	if( report_len == 0 ) {
		Log_Warning("USB_HID", "No report descriptor");
		LEAVE('-');
		return ;
	}
	
	// --- Read and parse report descriptor ---
	// NOTE: Also does sub-driver selection and initialisation
	Uint8	*report_data = alloca(report_len);
	USB_ReadDescriptor(Dev, 0x1022, 0, report_len, report_data);
	HID_int_ParseReport(Dev, report_data, report_len, &gHID_RootCallbacks);
	
	LEAVE('-');
}

/**
 * \brief Handle a Collection item in a report
 * 
 * When an application collection is found in the root of the tree, the approriate
 * sub-handler is invoked (mouse, keyboard, gamepad, ...)
 */
tHID_ReportCallbacks *HID_RootCollection(
	tUSBInterface *Dev,
	tHID_ReportGlobalState *Global, tHID_ReportLocalState *Local, Uint32 Value
	)
{
	Uint32	usage;
	
	// Check for "Application" collection
	if( Value != 1 )	return NULL;
	
	// Check usages
	if( Local->Usages.nItems == 0 )	return NULL;
	usage = Local->Usages.Items[0];
	switch(usage >> 16)
	{
	case 0x0001:	// General Desktop
		switch(usage & 0xFFFF)
		{
		case 0x0001:	// Pointer
			LOG("Desktop->Pointer");
			break;
		case 0x0002:	// Mouse
			LOG("Desktop->Mouse");
			break;
		case 0x0004:	// Joystick
		case 0x0005:	// Game Pad
			LOG("Desktop->Gamepad");
			break;
		case 0x0006:	// Keyboard
			LOG("Desktop->Keyboard");
			break;
		}
		break;
	case 0x0007:	// Keyboard / Keypad
		LOG("Keyboard");
		break;
	}
	return NULL;
}

void HID_int_ParseReport(tUSBInterface *Dev, Uint8 *Data, size_t Length, tHID_ReportCallbacks *StartCBs)
{
	const int	max_cb_level = 8;
	 int	cb_level = 0;
	tHID_ReportCallbacks	*cur_cbs;
	tHID_ReportCallbacks	*cb_stack[max_cb_level];
	tHID_ReportGlobalState	global_state;
	tHID_ReportLocalState	local_state;
	
	ENTER("pData iLength pStartCBs", Data, Length, StartCBs);

	// Initialise callback stack
	cb_stack[0] = StartCBs;
	cur_cbs = StartCBs;

	// Clear state
	memset(&global_state, 0, sizeof(global_state));
	memset(&local_state, 0, sizeof(local_state));

	// Iterate though the report data
	for( int ofs = 0; ofs < Length; )
	{
		Uint8	byte;
		Uint32	val;
		
		// --- Get value and length ---
		byte = Data[ofs];
		switch(byte & 3)
		{
		case 0:
			val = 0;
			ofs += 1;
			break;
		case 1:
			if( ofs + 2 > Length ) { LEAVE('-'); return; }
			val = Data[ofs+1];
			ofs += 2;
			break;
		case 2:
			if( ofs + 3 > Length ) { LEAVE('-'); return; }
			val = Data[ofs + 1] | (Data[ofs + 1]<<8);
			ofs += 3;
			break;
		case 3:
			if( ofs + 5 > Length ) { LEAVE('-'); return; }
			val = Data[ofs + 1] | (Data[ofs + 2] << 8) | (Data[ofs + 3] << 16) | (Data[ofs + 4] << 24);
			ofs += 5;
			break;
		}
	
		// --- Process the item ---
		LOG("Type = 0x%x, len = %i, val = 0x%x", byte & 0xFC, byte & 3, val);
		switch(byte & 0xFC)
		{
		// Main items
		// - Input
		case 0x80:
			LOG("Input 0x%x", val);
			if( cur_cbs && cur_cbs->Input )
				cur_cbs->Input( Dev, &global_state, &local_state, val );
			goto _clear_local;
		// - Output
		case 0x90:
			LOG("Output 0x%x", val);
			if( cur_cbs && cur_cbs->Output )
				cur_cbs->Output( Dev, &global_state, &local_state, val );
			goto _clear_local;
		// - Collection
		case 0xA0:
			LOG("Collection 0x%x", val);
			
			tHID_ReportCallbacks	*next_cbs = NULL;
			if( cur_cbs && cur_cbs->Collection )
				next_cbs = cur_cbs->Collection( Dev, &global_state, &local_state, val );
			cb_level ++;
			if( cb_level < max_cb_level )
				cb_stack[cb_level] = next_cbs;
			else
				next_cbs = NULL;
			cur_cbs = next_cbs;
			break;
		// - Feature
		case 0xB0:
			LOG("Feature 0x%x", val);
			if( cur_cbs && cur_cbs->Feature )
				cur_cbs->Feature( Dev, &global_state, &local_state, val );
			goto _clear_local;
		// - End collection
		case 0xC0:
			if( cur_cbs && cur_cbs->EndCollection )
				cur_cbs->EndCollection(Dev);
			if( cb_level == 0 ) {
				Log_Warning("USB_HID", "Inbalance in HID collections");
				LEAVE('-');
				return ;
			}
			cb_level --;
			if( cb_level < max_cb_level )
				cur_cbs = cb_stack[cb_level];
			goto _clear_local;
		// -- Helper to clear the local state
		_clear_local:
			_FreeList( &local_state.Strings );
			_FreeList( &local_state.Usages );
			_FreeList( &local_state.Designators );
			memset( &local_state, 0, sizeof(local_state) );
			break;
		
		// Global Items
		case 0x04:	global_state.UsagePage = val<<16;	break;	// - Usage Page
		case 0x14:	global_state.LogMin = val;	break;	// - Logical Min
		case 0x24:	global_state.LogMax = val;	break;	// - Logical Max
		case 0x34:	global_state.PhysMin = val;	break;	// - Physical Min
		case 0x44:	global_state.PhysMax = val;	break;	// - Physical Max
		case 0x54:	global_state.UnitExp = val;	break;	// - Unit Exp
		case 0x64:	global_state.Unit = val;	break;	// - Unit
		case 0x74:	global_state.ReportSize = val;	break;	// - Report Size
		case 0x84:	global_state.ReportID = val;	break;	// - Report ID (TODO: Flag when used)
		case 0x94:	global_state.ReportCount = val;	break;	// - Report Count
		case 0xA4:	LOG("TODO: Implement Global Push");	break;	// - Push
		case 0xB4:	LOG("TODO: Implement Global Pop");	break;	// - Pop
		
		// Local Items
		// - Usages
		case 0x08:	// Single
			if( (byte & 3) != 3 )	val |= global_state.UsagePage;
			_AddItem(&local_state.Usages, val);
			break;
		case 0x18:	// Range start
			if( (byte & 3) != 3 )	val |= global_state.UsagePage;
			local_state.UsageMin = val;
			break;
		case 0x28:	// Range end (apply)
			if( (byte & 3) != 3 )	val |= global_state.UsagePage;
			_AddItems(&local_state.Usages, local_state.UsageMin, val);
			break;
		// - Designators (Index into Physical report)
		case 0x38:
			_AddItem(&local_state.Designators, val);
			break;
		case 0x48:
			local_state.DesignatorMin = val;
			break;
		case 0x58:
			_AddItems(&local_state.Designators, local_state.DesignatorMin, val);
			break;
		// - Reserved/hole
		case 0x68:
			break;
		// - Strings
		case 0x78:
			_AddItem(&local_state.Strings, val);
			break;
		case 0x88:
			local_state.StringMin = val;
			break;
		case 0x98:
			_AddItems(&local_state.Strings, local_state.StringMin, val);
			break;
		// - Delimiter
		case 0xA8:
			break;
		
		// Long Item
		case 0xFC:
			LOG("Long Item");
			break;
		
		// Reserved
		default:
			LOG("0x%x RESVD", byte & 0xFC);
			break;
		}
	}

	LEAVE('-');
}

// --------------------------------------------------------------------
// List helpers
// --------------------------------------------------------------------
static void _AddItem(struct sHID_IntList *List, Uint32 Value)
{
	if( List->Space == List->nItems )
	{
		List->Space += 10;
		List->Items = realloc( List->Items, List->Space * sizeof(List->Items[0]) );
	}
	
	List->Items[ List->nItems ] = Value;
	List->nItems ++;
}

static void _AddItems(struct sHID_IntList *List, Uint32 First, Uint32 Last)
{
	while( First <= Last )
	{
		_AddItem(List, First);
		First ++;
	}
}

static void _FreeList(struct sHID_IntList *List)
{
	if( List->Items )
		free(List->Items);
}


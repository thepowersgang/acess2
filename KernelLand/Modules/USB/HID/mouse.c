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
#include <Input/Mouse/include/mouse.h>

// === CONSTANTS ===
#define MAX_AXIES	3	// X, Y, Scroll
#define MAX_BUTTONS	5	// Left, Right, Middle, ...

// === TYPES ===
typedef struct sHID_Mouse	tHID_Mouse;

struct sHID_Mouse
{
	tHID_Mouse	*Next;
	tUSB_DataCallback	DataAvail;

	tMouse	*Handle;

	// - Report parsing
	 int	nMappings;
	struct {
		Uint8	Dest;	// 0x00-0x7F = Buttons, 0x80-0xFE = Axies, 0xFF = Ignore
		Uint8	BitSize;
	} *Mappings;
	
	// - Initialisation only data
	 int	CollectionDepth;
};

// === PROTOTYES ===
Sint32	_ReadBits(void *Data, int Offset, int Length);
void	HID_Mouse_DataAvail(tUSBInterface *Dev, int EndPt, int Length, void *Data);

tHID_ReportCallbacks	*HID_Mouse_Report_Collection(tUSBInterface *Dev, tHID_ReportGlobalState *Global, tHID_ReportLocalState *Local, Uint32 Value);
void	HID_Mouse_Report_EndCollection(tUSBInterface *Dev);
void	HID_Mouse_Report_Input(tUSBInterface *Dev, tHID_ReportGlobalState *Global, tHID_ReportLocalState *Local, Uint32 Value);

// === GLOBALS ===
tHID_ReportCallbacks	gHID_Mouse_ReportCBs = {
	.Collection = HID_Mouse_Report_Collection,
	.EndCollection = HID_Mouse_Report_EndCollection,
	.Input = HID_Mouse_Report_Input,
};
tMutex	glHID_MouseListLock;
tHID_Mouse	*gpHID_FirstMouse;
tHID_Mouse	*gpHID_LastMouse = (tHID_Mouse*)&gpHID_FirstMouse;

// === CODE ===
// ----------------------------------------------------------------------------
// Data input / Update
// ----------------------------------------------------------------------------
/**
 * \brief Read a set amounts of bits from a stream
 * \param Data	Base of data
 * \param Offset	Bit offset
 * \param Length	Number of bits to read
 * \return Sign-extended value
 */
Sint32 _ReadBits(void *Data, int Offset, int Length)
{
	 int	dest_ofs = 0;
	 int	rem = Length;
	Uint32	rv = 0;
	Uint8	*bytes = (Uint8*)Data + Offset / 8;

	// Sanity please	
	if( Length > 32 )	return 0;

	// Leading byte
	if( Offset & 7 )
	{
		if( Length < 8 - (Offset & 7) )
		{
			rv = (*bytes >> Offset) & ((1 << Length)-1);
			goto _ext;
		}
		
		rv = (*bytes >> Offset);
		
		dest_ofs = Offset & 7;
		rem = Length - (Offset & 7);
		bytes ++;
	}

	// Body bytes
	while( rem >= 8 )
	{
		rv |= *bytes << dest_ofs;
		dest_ofs += 8;
		rem -= 8;
		bytes ++;
	}
	
	if( rem )
	{
		rv |= (*bytes & ((1 << rem)-1)) << dest_ofs;
	}
	
	// Do sign extension
_ext:
	if( rv & (1 << (Length-1)) )
		rv |= 0xFFFFFFFF << Length;
	return rv;
}

/**
 * \brief Handle an update from the device
 */
void HID_Mouse_DataAvail(tUSBInterface *Dev, int EndPt, int Length, void *Data)
{
	tHID_Mouse	*info;
	 int	ofs;
	Uint32	button_value = 0;
	 int	axis_values[MAX_AXIES] = {0};
	
	info = USB_GetDeviceDataPtr(Dev);
	if( !info )	return ;

	ofs = 0;
	for( int i = 0; i < info->nMappings; i ++ )
	{
		Sint32	value;
		Uint8	dest = info->Mappings[i].Dest;

		if( ofs + info->Mappings[i].BitSize > Length * 8 )
			return ;

		value = _ReadBits(Data, ofs, info->Mappings[i].BitSize);
		LOG("%i+%i: value = %i", ofs, info->Mappings[i].BitSize, value);
		ofs += info->Mappings[i].BitSize;
		
		if( dest == 0xFF )	continue ;
		
		if( dest & 0x80 )
		{
			// Axis
			axis_values[ dest & 0x7F ] = value;
			LOG("Axis %i = %i", dest & 0x7F, value);
		}
		else
		{
			// Button
			if( value == 0 )
				;
			else
				button_value |= 1 << (dest & 0x7F);
		}
	}
	
	Mouse_HandleEvent(info->Handle, button_value, axis_values);
}

// ----------------------------------------------------------------------------
// Device initialisation
// ----------------------------------------------------------------------------
/**
 * \brief Handle the opening of a collection
 */
tHID_ReportCallbacks *HID_Mouse_Report_Collection(
	tUSBInterface *Dev,
	tHID_ReportGlobalState *Global, tHID_ReportLocalState *Local,
	Uint32 Value
	)
{
	tHID_Mouse	*info;

	info = USB_GetDeviceDataPtr(Dev);
	if( !info )
	{
		// New device!
		info = calloc( sizeof(tHID_Mouse), 1 );
		info->DataAvail = HID_Mouse_DataAvail;
	
		info->Handle = Mouse_Register("USBMouse", MAX_BUTTONS, MAX_AXIES);
	
		LOG("Initialised new mouse at %p", info);
		
		USB_SetDeviceDataPtr(Dev, info);
	}
	else
	{
		info->CollectionDepth ++;
	}
	
	return &gHID_Mouse_ReportCBs;
}

/**
 * \brief Handle the end of a collection
 */
void HID_Mouse_Report_EndCollection(tUSBInterface *Dev)
{
	tHID_Mouse	*info;

	info = USB_GetDeviceDataPtr(Dev);	
	if(!info) {
		Log_Error("HID", "HID is not initialised when _AddInput is called");
		return ;
	}

	if( info->CollectionDepth == 0 )
	{
		// Perform final initialisation steps
		Mutex_Acquire(&glHID_MouseListLock);
		gpHID_LastMouse->Next = info;
		gpHID_LastMouse = info;
		Mutex_Release(&glHID_MouseListLock);
	}
	else
	{
		info->CollectionDepth --;
	}
}

/**
 * \brief Add a new input mapping
 */
void HID_int_AddInput(tUSBInterface *Dev, Uint32 Usage, Uint8 Size, Uint32 Min, Uint32 Max)
{
	Uint8	tag;
	tHID_Mouse	*info;

	info = USB_GetDeviceDataPtr(Dev);
	if(!info) {
		Log_Error("HID", "HID is not initialised when _AddInput is called");
		return ;
	}

	// --- Get the destination for the field ---
	switch(Usage)
	{
	case 0x00010030:	tag = 0x80;	break;	// Generic Desktop - X
	case 0x00010031:	tag = 0x81;	break;	// Generic Desktop - Y
	case 0x00010038:	tag = 0x82;	break;	// Generic Desktop - Wheel
	case 0x00090001:	tag = 0x00;	break;	// Button 1
	case 0x00090002:	tag = 0x01;	break;	// Button 2
	case 0x00090003:	tag = 0x02;	break;	// Button 3
	case 0x00090004:	tag = 0x03;	break;	// Button 4
	case 0x00090005:	tag = 0x04;	break;	// Button 5
	default:	tag = 0xFF;	break;
	}
	
	LOG("Usage = 0x%08x, tag = 0x%2x", Usage, tag);

	// --- Add to list of mappings ---
	info->nMappings ++;
	info->Mappings = realloc(info->Mappings, info->nMappings * sizeof(info->Mappings[0]));
	// TODO: NULL check
	info->Mappings[ info->nMappings - 1].Dest = tag;
	info->Mappings[ info->nMappings - 1].BitSize = Size;
	
	// --- Update Min/Max for Axies ---
	// TODO: DPI too
	// TODO: Pass to mouse multiplexer
//	if( tag != 0xFF && (tag & 0x80) )
//	{
//		info->Axies[ tag & 0x7F ].MinValue = Min;
//		info->Axies[ tag & 0x7F ].MaxValue = Max;
//	}
}

/**
 * \brief Handle an input item in a report
 */
void HID_Mouse_Report_Input(
	tUSBInterface *Dev,
	tHID_ReportGlobalState *Global, tHID_ReportLocalState *Local,
	Uint32 Value
	)
{
	Uint32	usage = 0;
	LOG("Local->Usages.nItems = %i", Local->Usages.nItems);
	for( int i = 0; i < Global->ReportCount; i ++ )
	{
		// - Update usage
		if( i < Local->Usages.nItems )
			usage = Local->Usages.Items[i];
		LOG("%i: usage = %x", i, usage);
		// - Add to list
		HID_int_AddInput(Dev, usage, Global->ReportSize, Global->LogMin, Global->LogMax);
	}
}


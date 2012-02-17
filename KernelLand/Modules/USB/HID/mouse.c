/*
 * Acess2 USB Stack HID Driver
 * - By John Hodge (thePowersGang)
 *
 * mouse.c
 * - USB Mouse driver
 */
#define DEBUG	1
#include <acess.h>
#include "hid_reports.h"
#include <fs_devfs.h>
#include <api_drv_joystick.h>

// === CONSTANTS ===
#define MAX_AXIES	3	// X, Y, Scroll
#define MAX_BUTTONS	5	// Left, Right, Middle, ...
#define FILE_SIZE	(sizeof(tJoystick_FileHeader) + MAX_AXIES*sizeof(tJoystick_Axis) + MAX_BUTTONS)

// === TYPES ===
typedef struct sHID_Mouse	tHID_Mouse;

struct sHID_Mouse
{
	tHID_Mouse	*Next;
	tUSB_DataCallback	DataAvail;

	// VFS Node
	tVFS_Node	Node;

	// Joystick Spec data	
	Uint8	FileData[ FILE_SIZE ];
	tJoystick_FileHeader	*FileHeader;
	tJoystick_Axis	*Axies;
	Uint8	*Buttons;

	// Limits for axis positions
	Uint16	AxisLimits[MAX_AXIES];
	
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
char	*HID_Mouse_Root_ReadDir(tVFS_Node *Node, int Pos);
tVFS_Node	*HID_Mouse_Root_FindDir(tVFS_Node *Node, const char *Name);
size_t	HID_Mouse_Dev_Read(tVFS_Node *Node, off_t Offset, size_t Length, void *Buffer);
 int	HID_Mouse_Dev_IOCtl(tVFS_Node *Node, int ID, void *Data);
void	HID_Mouse_Dev_Reference(tVFS_Node *Node);
void	HID_Mouse_Dev_Close(tVFS_Node *Node);

void	HID_Mouse_DataAvail(tUSBInterface *Dev, int EndPt, int Length, void *Data);

tHID_ReportCallbacks	*HID_Mouse_Report_Collection(tUSBInterface *Dev, tHID_ReportGlobalState *Global, tHID_ReportLocalState *Local, Uint32 Value);
void	HID_Mouse_Report_EndCollection(tUSBInterface *Dev);
void	HID_Mouse_Report_Input(tUSBInterface *Dev, tHID_ReportGlobalState *Global, tHID_ReportLocalState *Local, Uint32 Value);

// === GLOBALS ===
tVFS_NodeType	gHID_Mouse_RootNodeType = {
	.TypeName = "HID Mouse Root",
	.ReadDir = HID_Mouse_Root_ReadDir,
	.FindDir = HID_Mouse_Root_FindDir
};
tVFS_NodeType	gHID_Mouse_DevNodeType = {
	.TypeName = "HID Mouse Dev",
	.Read = HID_Mouse_Dev_Read,
	.IOCtl = HID_Mouse_Dev_IOCtl,
	.Reference = HID_Mouse_Dev_Reference,
	.Close = HID_Mouse_Dev_Close,
};
tDevFS_Driver	gHID_Mouse_DevFS = {
	.Name = "USBMouse",
	.RootNode = {
		.Type = &gHID_Mouse_RootNodeType,
		.Flags = VFS_FFLAG_DIRECTORY
	}
};
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
// VFS Interface
// ----------------------------------------------------------------------------
char *HID_Mouse_Root_ReadDir(tVFS_Node *Node, int Pos)
{
	char	data[3];
	if(Pos < 0 || Pos >= Node->Size)	return NULL;
	
	snprintf(data, 3, "%i", Pos);
	return strdup(data);
}

tVFS_Node *HID_Mouse_Root_FindDir(tVFS_Node *Node, const char *Name)
{
	 int	ID;
	 int	ofs;
	tHID_Mouse	*mouse;
	
	if( Name[0] == '\0' )
		return NULL;
	
	ofs = ParseInt(Name, &ID);
	if( ofs == 0 || Name[ofs] != '\0' )
		return NULL;
	
	// Scan list, locate item
	Mutex_Acquire(&glHID_MouseListLock);
	for( mouse = gpHID_FirstMouse; mouse && ID --; mouse = mouse->Next ) ;
	mouse->Node.ReferenceCount ++;	
	Mutex_Release(&glHID_MouseListLock);

	return &mouse->Node;
}

size_t HID_Mouse_Dev_Read(tVFS_Node *Node, off_t Offset, size_t Length, void *Buffer)
{
	tHID_Mouse	*info = Node->ImplPtr;
	
	if( Offset > FILE_SIZE )	return 0;
	
	if( Length > FILE_SIZE )	Length = FILE_SIZE;
	if( Offset + Length > FILE_SIZE )	Length = FILE_SIZE - Offset;

	memcpy( Buffer, info->FileData + Offset, Length );

	return Length;
}

static const char *csaDevIOCtls[] = {DRV_IOCTLNAMES, DRV_JOY_IOCTLNAMES, NULL};
int HID_Mouse_Dev_IOCtl(tVFS_Node *Node, int ID, void *Data)
{
	switch(ID)
	{
	BASE_IOCTLS(DRV_TYPE_JOYSTICK, "USBMouse", 0x050, csaDevIOCtls);
	}
	return -1;
}
void HID_Mouse_Dev_Reference(tVFS_Node *Node)
{
	Node->ReferenceCount ++;
}
void HID_Mouse_Dev_Close(tVFS_Node *Node)
{
	Node->ReferenceCount --;
}

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
		Length -= Offset & 7;
		bytes ++;
	}

	// Body bytes
	while( Length >= 8 )
	{
		rv |= *bytes << dest_ofs;
		dest_ofs += 8;
		Length -= 8;
		bytes ++;
	}
	
	if( Length )
	{
		rv |= (*bytes & ((1 << Length)-1)) << dest_ofs;
		
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
	
	info = USB_GetDeviceDataPtr(Dev);
	if( !info )	return ;

	Log_Debug("USBMouse", "info = %p", info);	
	
	ofs = 0;
	for( int i = 0; i < info->nMappings; i ++ )
	{
		Sint32	value;
		Uint8	dest = info->Mappings[i].Dest;

		if( ofs + info->Mappings[i].BitSize > Length * 8 )
			return ;

		value = _ReadBits(Data, ofs, info->Mappings[i].BitSize);
		ofs += info->Mappings[i].BitSize;
		
		if( dest == 0xFF )	continue ;
		
		if( dest & 0x80 )
		{
			// Axis
			info->Axies[ dest & 0x7F ].CurValue = value;
		}
		else
		{
			// Button
			info->Buttons[ dest & 0x7F ] = (value == 0) ? 0 : 0xFF;
		}
	}
	
	// Update axis positions
	for( int i = 0; i < MAX_AXIES; i ++ )
	{
		 int	newpos;
		
		// TODO: Scaling
		newpos = info->Axies[i].CursorPos + info->Axies[i].CurValue;
		
		if(newpos < 0)	newpos = 0;
		if(newpos > info->AxisLimits[i])	newpos = info->AxisLimits[i];
		
		info->Axies[i].CursorPos = newpos;
	}
	Log_Debug("USBMouse", "New Pos (%i,%i,%i)",
		info->Axies[0].CursorPos, info->Axies[1].CursorPos, info->Axies[2].CursorPos
		);
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
		info->Node.ImplPtr = info;
		info->Node.Type = &gHID_Mouse_DevNodeType;
		
		info->FileHeader = (void*)info->FileData;
		info->Axies = (void*)(info->FileHeader + 1);
		info->Buttons = (void*)(info->Axies + MAX_AXIES);
	
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
		gHID_Mouse_DevFS.RootNode.Size ++;
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
	
	// --- Add to list of mappings ---
	info->nMappings ++;
	info->Mappings = realloc(info->Mappings, info->nMappings * sizeof(info->Mappings[0]));
	// TODO: NULL check
	info->Mappings[ info->nMappings - 1].Dest = tag;
	info->Mappings[ info->nMappings - 1].BitSize = Size;
	
	// --- Update Min/Max for Axies ---
	// TODO: DPI too
	if( tag != 0xFF && (tag & 0x80) )
	{
		info->Axies[ tag & 0x7F ].MinValue = Min;
		info->Axies[ tag & 0x7F ].MaxValue = Max;
	}
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
	for( int i = 0; i < Global->ReportCount; i ++ )
	{
		// - Update usage
		if( i < Local->Usages.nItems )
			usage = Local->Usages.Items[i];
		// - Add to list
		HID_int_AddInput(Dev, usage, Global->ReportSize, Global->LogMin, Global->LogMax);
	}
}


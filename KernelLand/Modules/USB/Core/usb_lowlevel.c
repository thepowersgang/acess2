/*
 * Acess 2 USB Stack
 * - By John Hodge (thePowersGang)
 * 
 * usb_lowlevel.c
 * - Low Level IO
 */
#define DEBUG	0
#include <acess.h>
#include "usb.h"
#include "usb_proto.h"
#include "usb_lowlevel.h"
#include <timers.h>
#include <events.h>

// === PROTOTYPES ===
void	*USB_int_Request(tUSBDevice *Dev, int EndPt, int Type, int Req, int Val, int Indx, int Len, void *Data);
void	USB_int_WakeThread(void *Thread, void *Data, size_t Length);
 int	USB_int_SendSetupSetAddress(tUSBHost *Host, int Address);
 int	USB_int_ReadDescriptor(tUSBDevice *Dev, int Endpoint, int Type, int Index, int Length, void *Dest);
char	*USB_int_GetDeviceString(tUSBDevice *Dev, int Endpoint, int Index);
 int	_UTF16to8(Uint16 *Input, int InputLen, char *Dest);

// === CODE ===
void *USB_int_Request(tUSBDevice *Device, int EndPt, int Type, int Req, int Val, int Indx, int Len, void *Data)
{
	tUSBHost	*Host = Device->Host;
	void	*hdl;
	// TODO: Sanity check (and check that Type is valid)
	struct sDeviceRequest	req;
	tThread	*thisthread = Proc_GetCurThread();
	void *dest_hdl;

	ENTER("pDevice xEndPt iType iReq iVal iIndx iLen pData",
		Device, EndPt, Type, Req, Val, Indx, Len, Data);
	
	if( EndPt < 0 || EndPt >= 16 ) {
		LEAVE('n');
		return NULL;
	}

	dest_hdl = Device->EndpointHandles[EndPt];
	if( !dest_hdl ) {
		LEAVE('n');
		return NULL;
	}
	
	req.ReqType = Type;
	req.Request = Req;
	req.Value = LittleEndian16( Val );
	req.Index = LittleEndian16( Indx );
	req.Length = LittleEndian16( Len );

	Threads_ClearEvent(THREAD_EVENT_SHORTWAIT);

	LOG("Send");
	if( Type & 0x80 ) {
		// Inbound data
		hdl = Host->HostDef->SendControl(Host->Ptr, dest_hdl, USB_int_WakeThread, thisthread, 0,
			&req, sizeof(req),
			NULL, 0,
			Data, Len
			);
	}
	else {
		// Outbound data
		hdl = Host->HostDef->SendControl(Host->Ptr, dest_hdl, USB_int_WakeThread, thisthread, 1,
			&req, sizeof(req),
			Data, Len,
			NULL, 0
			);
	}
	LOG("Wait...");
	Threads_WaitEvents(THREAD_EVENT_SHORTWAIT);

	LEAVE('p', hdl);
	return hdl;
}

void USB_int_WakeThread(void *Thread, void *Data, size_t Length)
{
	Threads_PostEvent(Thread, THREAD_EVENT_SHORTWAIT);
}

int USB_int_SendSetupSetAddress(tUSBHost *Host, int Address)
{
	USB_int_Request(&Host->RootHubDev, 0, 0x00, 5, Address & 0x7F, 0, 0, NULL);
	return 0;
}

int USB_int_ReadDescriptor(tUSBDevice *Dev, int Endpoint, int Type, int Index, int Length, void *Dest)
{
	struct sDeviceRequest	req;
	void	*dest_hdl;
	
	dest_hdl = Dev->EndpointHandles[Endpoint];
	if( !dest_hdl ) {
		return -1;
	}

	ENTER("pDev xEndpoint iType iIndex iLength pDest",
		Dev, Endpoint, Type, Index, Length, Dest);

	req.ReqType = 0x80;
	req.ReqType |= ((Type >> 8) & 0x3) << 5;	// Bits 5/6
	req.ReqType |= (Type >> 12) & 3;	// Destination (Device, Interface, Endpoint, Other);

	req.Request = 6;	// GET_DESCRIPTOR
	req.Value = LittleEndian16( ((Type & 0xFF) << 8) | (Index & 0xFF) );
	req.Index = LittleEndian16( 0 );	// TODO: Language ID / Interface
	req.Length = LittleEndian16( Length );

	Threads_ClearEvent(THREAD_EVENT_SHORTWAIT);
	
	LOG("Send");
	Dev->Host->HostDef->SendControl(Dev->Host->Ptr, dest_hdl, USB_int_WakeThread, Proc_GetCurThread(), 0,
		&req, sizeof(req),
		NULL, 0,
		Dest, Length
		);

	LOG("Waiting");
	// TODO: Detect errors?
	Threads_WaitEvents(THREAD_EVENT_SHORTWAIT);
	
	LEAVE('i', 0);
	return 0;
}

char *USB_int_GetDeviceString(tUSBDevice *Dev, int Endpoint, int Index)
{
	struct sDescriptor_String	str;
	 int	src_len, new_len;
	char	*ret;

	if(Index == 0)	return strdup("");
	
	str.Length = 0;
	USB_int_ReadDescriptor(Dev, Endpoint, 3, Index, sizeof(str), &str);
	if(str.Length == 0)	return NULL;
	if(str.Length < 2) {
		Log_Error("USB", "String %p:%i:%i:%i descriptor is undersized (%i)",
			Dev->Host, Dev->Address, Endpoint, Index, str.Length);
		return NULL;
	}
//	if(str.Length > sizeof(str)) {
//		// IMPOSSIBLE!
//		Log_Error("USB", "String is %i bytes, which is over prealloc size (%i)",
//			str.Length, sizeof(str)
//			);
//	}
	src_len = (str.Length - 2) / sizeof(str.Data[0]);

	LOG("&str = %p, src_len = %i", &str, src_len);

	new_len = _UTF16to8(str.Data, src_len, NULL);	
	ret = malloc( new_len + 1 );
	_UTF16to8(str.Data, src_len, ret);
	ret[new_len] = 0;
	return ret;
}

int _UTF16to8(Uint16 *Input, int InputLen, char *Dest)
{
	 int	str_len, cp_len;
	Uint32	saved_bits = 0;
	str_len = 0;
	for( int i = 0; i < InputLen; i ++)
	{
		Uint32	cp;
		Uint16	val = Input[i];
		if( val >= 0xD800 && val <= 0xDBFF )
		{
			// Multibyte - Leading
			if(i + 1 > InputLen) {
				cp = '?';
			}
			else {
				saved_bits = (val - 0xD800) << 10;
				saved_bits += 0x10000;
				continue ;
			}
		}
		else if( val >= 0xDC00 && val <= 0xDFFF )
		{
			if( !saved_bits ) {
				cp = '?';
			}
			else {
				saved_bits |= (val - 0xDC00);
				cp = saved_bits;
			}
		}
		else
			cp = val;

		cp_len = WriteUTF8((Uint8*)Dest, cp);
		if(Dest)
			Dest += cp_len;
		str_len += cp_len;

		saved_bits = 0;
	}
	
	return str_len;
}


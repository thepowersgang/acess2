/*
 * Acess2 USB Stack - HID Driver
 * - By John Hodge (thePowersGang)
 *
 * hid_reports.h
 * - Report parsing definitions
 */
#ifndef _USB_HID_REPORTS_H_
#define _USB_HID_REPORTS_H_

#include <usb_core.h>

typedef struct sHID_ReportCallbacks	tHID_ReportCallbacks;
typedef struct sHID_ReportGlobalState	tHID_ReportGlobalState;
typedef struct sHID_ReportLocalState	tHID_ReportLocalState;

struct sHID_ReportCallbacks
{
	void	(*Input)(
		tUSBInterface *Dev,
		tHID_ReportGlobalState *Global, tHID_ReportLocalState *Local, Uint32 Data
		);
	void	(*Output)(
		tUSBInterface *Dev,
		tHID_ReportGlobalState *Global, tHID_ReportLocalState *Local, Uint32 Data
		);
	void	(*Feature)(
		tUSBInterface *Dev,
		tHID_ReportGlobalState *Global, tHID_ReportLocalState *Local, Uint32 Data
		);
	
	tHID_ReportCallbacks	*(*Collection)(
		tUSBInterface *Dev,
		tHID_ReportGlobalState *Global, tHID_ReportLocalState *Local, Uint32 Data
		);
	
	void	(*EndCollection)(tUSBInterface *Dev);
};

struct sHID_ReportGlobalState
{
	Uint32	UsagePage;
	Uint32	LogMin;
	Uint32	LogMax;
	Uint32	PhysMin;
	Uint32	PhysMax;
	
	Uint8	UnitExp;
	Uint8	Unit;
	
	Uint32	ReportSize;
	Uint32	ReportID;
	Uint32	ReportCount;
};

struct sHID_IntList
{
	 int	Space;
	 int	nItems;
	Uint32	*Items;
};

struct sHID_ReportLocalState
{
	struct sHID_IntList	Usages;
	Uint32	UsageMin;
	struct sHID_IntList	Designators;
	Uint32	DesignatorMin;
	struct sHID_IntList	Strings;
	Uint32	StringMin;

};

#endif


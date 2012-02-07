/*
 * Acess2 USB Stack HID Driver
 * - By John Hodge (thePowersGang)
 *
 * main.c
 * - Driver Core
 */
#define DEBUG	0
#define VERSION	VER2(0,1)
#include <acess.h>
#include <usb_core.h>

// === PROTOTYPES ===
 int	HID_Initialise(const char **Arguments);
void	HID_DeviceConnected(tUSBInterface *Dev);

// === GLOBALS ===
MODULE_DEFINE(0, VERSION, USB_HID, HID_Initialise, NULL, "USB_Core", NULL);
tUSBDriver	gHID_Driver = {
	.Name = "HID",
	.Match = {.Class = {0x030000, 0xFF0000}},
	.Connected = HID_DeviceConnected,
};

// === CODE ===
int HID_Initialise(const char **Arguments)
{
	USB_RegisterDriver( &gHID_Driver );
	return 0;
}

void HID_DeviceConnected(tUSBInterface *Dev)
{
	
}


/*
 AcessOS PCI Dump
*/
#include <acess/sys.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define DUMP_BARS	0

#include "pcilist.h"

// === CONSTANTS ===
const struct {
	 int	ID;
	char	*Name;
} csaVENDORS[] = {
	{0x10EC, "Realtek"},
	{0x1106, "VIA Technologies, Inc."},
	{0x8086, "Intel"}
};
#define VENDOR_COUNT	(sizeof(csaVENDORS)/sizeof(csaVENDORS[0]))

// === PROTOTYPES ===
char	*GetVendorName(uint16_t ID);
char	*GetDeviceName(uint16_t ven, uint16_t dev);

// ==== CODE ====
int main(int argc, char *argv[], char *envp[])
{
	 int	dp, fp;
	char	tmpPath[256+13] = "/Devices/pci/";
	char	*fileName = tmpPath + sizeof("/Devices/pci");
	uint16_t	vendor, device;
	#if DUMP_BARS
	uint32_t	tmp32;
	#endif
	
	printf("PCI Bus Dump\n");
	// --- Open PCI Directory
	dp = open("/Devices/pci", OPENFLAG_READ|OPENFLAG_EXEC);
	if(dp == -1)
	{
		fprintf(stderr, "Non-Standard configuration or not running on Acess.\n");
		fprintf(stderr, "Quitting - Reason: Unable to open PCI driver.\n");
		return -1;
	}
	
	// --- List Contents
	// Uses `fp` as a temp variable
	while( (fp = readdir(dp, fileName)) )
	{
		if(fp < 0) {
			if(fp == -3)	printf("Invalid Permissions to traverse directory\n");
			break;
		}
		
		printf("Bus %c%c, Index %c%c, Fcn %c: ", fileName[0],fileName[1],
			fileName[3],fileName[4], fileName[6]);
		
		// Open File
		fp = open(tmpPath, OPENFLAG_READ);
		if(fp == -1)	continue;
		
		read(fp, &vendor, 2);	read(fp, &device, 2);
		printf(" Vendor 0x%04x, Device 0x%04x\n", vendor, device);
		printf(" %s - %s\n", GetVendorName(vendor), GetDeviceName(vendor, device));
		
		// Reuse vendor and device
		seek(fp, 0x8, SEEK_SET);
		read(fp, &vendor, 2);	read(fp, &device, 2);
		printf(" Revision 0x%04x, Class 0x%04x ()\n", vendor, device);
		
		// Read File
		#if DUMP_BARS
		seek(fp, 0x10, SEEK_SET);
		printf("Base Address Registers (BARs):\n");
		read(fp, &tmp32, 4);	printf(" 0x%08x", tmp32);
		read(fp, &tmp32, 4);	printf(" 0x%08x", tmp32);
		read(fp, &tmp32, 4);	printf(" 0x%08x", tmp32);
		read(fp, &tmp32, 4);	printf(" 0x%08x", tmp32);
		read(fp, &tmp32, 4);	printf(" 0x%08x", tmp32);
		read(fp, &tmp32, 4);	printf(" 0x%08x", tmp32);
		printf("\n");
		#endif
		printf("\n");
		
		// Close File
		close(fp);
	}
	close(dp);
	return 0;
}

char *GetVendorName(uint16_t id)
{
	int i;
	#if 0
	for( i = 0; i < VENDOR_COUNT; i++)
	{
		if( csaVENDORS[i].ID == id )
			return csaVENDORS[i].Name;
	}
	#else
	for( i = 0; i < PCI_VENTABLE_LEN; i++ )
	{
		if( PciVenTable[i].VenId == id )
			return PciVenTable[i].VenFull;
	}
	#endif
	return "Unknown";
}

char *GetDeviceName(uint16_t ven, uint16_t dev)
{
	 int	i;
	for( i = 0; i < PCI_DEVTABLE_LEN; i++ )
	{
		if( PciDevTable[i].VenId == ven && PciDevTable[i].DevId == dev )
			return PciDevTable[i].ChipDesc;
	}
	return "Unknown";
}

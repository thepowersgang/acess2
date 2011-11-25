/*
 * Acess2 USB Stack HID Driver
 * - By John Hodge (thePowersGang)
 *
 * hid.h
 * - Core header
 */
#ifndef _HID_H_
#define _HID_H_

// Report Descriptor Types
// - 0: Main
//  > 8: Input - Axis/Button etc
//  > 9: Output - LED/Position
//  > B: Feature -
//  > A: Collection - Group of Input/Output/Feature
//  > C: End collection
// - 1: Global (Not restored on main)
// - 2: Local
//  > 
// - 3: Reserved/Unused

// I/O/F Data Values
#define HID_IOF_CONSTANT	0x001	//!< Host Read-only
#define HID_IOF_VARIABLE	0x002	//!< 

struct sLongItem
{
	Uint8	Header;	// 0xFE (Tag 15, Type 3, Size 2)
	Uint8	DataSize;
	Uint8	Tag;
};

struct sDescriptor_HID
{
	Uint8	Length;
	Uint8	Type;	// 
	Uint16	Version;	// 0x0111 = 1.11
	Uint8	CountryCode;
	Uint8	NumDescriptors;	// >= 1
	struct {
		Uint8	DescType;
		Uint16	DescLen;
	} Descriptors[];
}

#endif

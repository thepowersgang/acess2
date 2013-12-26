/*
 * Acess2 Kernel VBE Driver
 * - By John Hodge (thePowersGang)
 *
 * vbe.h
 * - Definitions for VBE structures
 */
#ifndef _VBE_H_
#define _VBE_H_

typedef struct sFarPtr
{
	Uint16	ofs;
	Uint16	seg;
}	tFarPtr;

typedef struct sVesa_CallModeInfo
{
	/**
	 * 0 : Mode is supported
	 * 1 : Optional information avaliable (Xres onwards)
	 * 2 : BIOS Output Supported
	 * 3 : Colour Mode?
	 * 4 : Graphics mode?
	 * -- VBE v2.0+
	 * 5 : Mode is not VGA compatible
	 * 6 : Bank switched mode supported
	 * 7 : Linear framebuffer mode supported
	 * 8 : Double-scan mode avaliable
	 */
	Uint16	attributes;
	/**
	 * 0 : Window exists
	 * 1 : Window is readable
	 * 2 : Window is writable
	 */
	Uint8	winA,winB;
	Uint16	granularity;
	Uint16	winsize;
	Uint16	segmentA, segmentB;
	tFarPtr	realFctPtr;
	Uint16	pitch;	// Bytes per scanline

	Uint16	Xres, Yres;
	Uint8	Wchar, Ychar, planes, bpp, banks;
	Uint8	memory_model, bank_size, image_pages;
	Uint8	reserved0;

	// -- VBE v1.2+
	Uint8	red_mask, red_position;
	Uint8	green_mask, green_position;
	Uint8	blue_mask, blue_position;
	Uint8	rsv_mask, rsv_position;
	Uint8	directcolor_attributes;

	// -- VBE v2.0+
	Uint32	physbase;
	Uint32	offscreen_ptr;	// Start of offscreen memory
	Uint16	offscreen_size_kb;	// Size of offscreen memory
	
	// -- VBE v3.0
	Uint16	lfb_pitch;
	Uint8	image_count_banked;
	Uint8	image_count_lfb;
} PACKED	tVesa_CallModeInfo;

typedef struct sVesa_CallInfo
{
	char		signature[4];		// == "VESA"
	Uint16	Version;	// == 0x0300 for Vesa 3.0
	tFarPtr	OEMString;	// isa vbeFarPtr
	Uint8	Capabilities[4];	
	tFarPtr	VideoModes;	// isa vbeParPtr
	Uint16	TotalMemory;	// as # of 64KB blocks
} PACKED	tVesa_CallInfo;


#endif


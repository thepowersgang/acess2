/**
 */
#ifndef _COMMON_H_
#define _COMMON_H_

// === TYPES ===
typedef struct sFarPtr
{
	Uint16	ofs;
	Uint16	seg;
}	tFarPtr;

typedef struct sVesa_Mode
{
	Uint16	code;
	Uint16	width, height;
	Uint16	pitch, bpp;
	Uint16	flags;
	Uint32	fbSize;
	Uint32	framebuffer;
}	tVesa_Mode;

typedef struct sVesa_CallModeInfo
{
	Uint16	attributes;
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

	Uint8	red_mask, red_position;
	Uint8	green_mask, green_position;
	Uint8	blue_mask, blue_position;
	Uint8	rsv_mask, rsv_position;
	Uint8	directcolor_attributes;

	Uint32	physbase;  // Your LFB address ;)
	Uint32	reserved1;
	Sint16	reserved2;
}	tVesa_CallModeInfo;

typedef struct sVesa_CallInfo
{
   char		signature[4];		// == "VESA"
   Uint16	Version;	// == 0x0300 for Vesa 3.0
   tFarPtr	OEMString;	// isa vbeFarPtr
   Uint8	Capabilities[4];	
   tFarPtr	VideoModes;	// isa vbeParPtr
   Uint16	TotalMemory;	// as # of 64KB blocks
}	tVesa_CallInfo;

#endif

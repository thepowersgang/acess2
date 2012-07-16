/*
 * Acess2 VIA Video Driver
 * - By John Hodge
 * 
 * main.c
 * - Driver core
 *
 * NOTE: Based off the UniChrome driver for X, http://unichrome.sourceforge.net/
 */
#define DEBUG	0
#include <acess.h>
#include <fs_devfs.h>
#include <drv_pci.h>
#include <modules.h>
#include <api_drv_video.h>
#include <timers.h>
#include "common.h"

#define VERSION	VER2(0,1)

// === CONSTANTS ===
const struct sVGA_Timings
{
	 int	HFP, HSync, HDisplay, HBP;
	 int	VFP, VSync, VDisplay, VBP;
} csaTimings[] = {
	{40, 128,  800,  88,  1, 4,  600, 23},	// SVGA @ 60Hz
	{24, 136, 1024, 160,  3, 6,  768, 29},	// XGA @ 60Hz
	{38, 112, 1280, 248,  1, 3, 1024, 38}	// 1280x1024 @ 60Hz
};
const Uint16	caVIAVideo_CardIDs[][2] = {
	{0x1106, 0x3108},	// minuet (unk chip)
	{0x1106, 0x3157},	// CX700/VX700 (S3 UniChrome Pro)
};
const int	ciVIAVideo_NumCardIDs = sizeof(caVIAVideo_CardIDs)/sizeof(caVIAVideo_CardIDs[0]);

// === PROTOTYPES ===
 int	VIAVideo_Initialise(char **Arguments);
void	VIAVideo_Cleanup(void);

size_t	VIAVideo_Write(tVFS_Node *Node, off_t Offset, size_t Length, const void *Buffer);
 int	VIAVideo_IOCtl(tVFS_Node *Node, int ID, void *Data);

void	VIAVideo_int_SetMode(const struct sVGA_Timings *Timings, int Depth);
void	VIAVideo_int_ResetCRTC(int Index, BOOL Reset);

static void	_SRMask(int Reg, Uint8 Byte, Uint8 Mask);
static void	_CRMask(int Reg, Uint8 Byte, Uint8 Mask);

// === GLOBALS ===
MODULE_DEFINE(0, VERSION, VIAVideo, VIAVideo_Initialise, VIAVideo_Cleanup, NULL);
tVFS_NodeType	gVIAVideo_NodeType = {
	.Write = VIAVideo_Write,
	.IOCtl = VIAVideo_IOCtl
	};
tDevFS_Driver	gVIAVideo_DriverStruct = {
	NULL, "VIAVideo",
	{.Type = &gVIAVideo_NodeType}
	};
 int	giVIAVideo_DriverID;
tVIAVideo_Dev	gVIAVideo_Info;

// === CODE ===
int VIAVideo_Initialise(char **Arguments)
{
	 int	count = 0;
	
	for( int i = 0; i < ciVIAVideo_NumCardIDs; i ++ )
	{
		count += PCI_CountDevices(caVIAVideo_CardIDs[i][0], caVIAVideo_CardIDs[i][1]);
	}
	if(count == 0)	return MODULE_ERR_NOTNEEDED;
	

	for( int i = 0; i < ciVIAVideo_NumCardIDs; i ++ )
	{
		for( int id = 0; (id = PCI_GetDevice(caVIAVideo_CardIDs[i][0], caVIAVideo_CardIDs[i][1], id)) != -1; id ++ )
		{
			// TODO: Support MMIO
			Log_Log("VIAVideo", "BAR0 = 0x%x", PCI_GetBAR(id, 0));
			Log_Log("VIAVideo", "BAR1 = 0x%x", PCI_GetBAR(id, 1));
			Log_Log("VIAVideo", "BAR2 = 0x%x", PCI_GetBAR(id, 2));
			Log_Log("VIAVideo", "BAR3 = 0x%x", PCI_GetBAR(id, 3));
			Log_Log("VIAVideo", "BAR4 = 0x%x", PCI_GetBAR(id, 4));
			Log_Log("VIAVideo", "BAR5 = 0x%x", PCI_GetBAR(id, 5));
			
			// Ignore multiple cards
			if( gVIAVideo_Info.FramebufferPhys )	continue ;
			
			gVIAVideo_Info.FramebufferPhys = PCI_GetBAR(id, 0);
			gVIAVideo_Info.MMIOPhys = PCI_GetBAR(id, 1);
			
			gVIAVideo_Info.Framebuffer = (void*)MM_MapHWPages(
				gVIAVideo_Info.FramebufferPhys, (1024*768*4)/PAGE_SIZE
				);
			// TODO: Map MMIO

			Uint8	tmp;
			tmp = PCI_ConfigRead(id, 0xA1, 1);
			Uint32	vidram = (1 << ((tmp & 0x70) >> 4)) * 4096;
			Log_Debug("VIAVideo", "0x%x bytes of video ram?", vidram);


			// Enable Framebuffer
			_SRMask(0x02, 0x0F, 0xFF);	/* Enable writing to all VGA memory planes */
			_SRMask(0x04, 0x0E, 0xFF);	/* Enable Extended VGA memory */
			_SRMask(0x1A, 0x08, 0x08);	/* Enable Extended Memory access */

			gVIAVideo_Info.BufInfo.Framebuffer = gVIAVideo_Info.Framebuffer;
			gVIAVideo_Info.BufInfo.Pitch = 1024*4;
			gVIAVideo_Info.BufInfo.Width = 1024;
			gVIAVideo_Info.BufInfo.Height = 768;
			gVIAVideo_Info.BufInfo.Depth = 32;
		}
	}

	VIAVideo_int_SetMode( &csaTimings[1], 32 );
	for( int i = 0; i < 768; i ++ )
	{
		Uint32	*scanline = gVIAVideo_Info.Framebuffer + i * 0x1000;
		for( int j = 0; j < 200; j ++ )
		{
			scanline[j] = 0xFFA06000;
		}
		Time_Delay(500);
	}

	// Install Device
	giVIAVideo_DriverID = DevFS_AddDevice( &gVIAVideo_DriverStruct );
	if(giVIAVideo_DriverID == -1)	return MODULE_ERR_MISC;

	return MODULE_ERR_OK;
}

void VIAVideo_Cleanup(void)
{
	return ;
}

size_t VIAVideo_Write(tVFS_Node *Node, off_t Offset, size_t Length, const void *Buffer)
{
	#if 0
	return DrvUtil_Video_WriteLFB(&gVIAVideo_Info.BufInfo, Offset, Length, Buffer);
	#endif
	return 0;
	#if 0
	if( Offset >= gVIAVideo_Info.FBSize )
		return 0;
	if( Length > gVIAVideo_Info.FBSize )
		Length = gVIAVideo_Info.FBSize;
	if( Offset + Length > gVIAVideo_Info.FBSize )
		Length = gVIAVideo_Info.FBSize - Offset;
	
	memcpy( (Uint8*)gVIAVideo_Info.Framebuffer + Offset, Buffer, Length );
	
	return Length;
	#endif
}

int VIAVideo_IOCtl(tVFS_Node *Node, int ID, void *Data)
{
	return 0;
}

// --- Modeset!
void VIAVideo_int_SetMode(const struct sVGA_Timings *Timings, int Depth)
{
	 int	temp;

	// Disable power
	_CRMask(0x36, 0x30, 0x30);	// CRT Off (TODO: Others)
	// VQ Disable (optional?)
	
	// Unlock registers
	_CRMask(0x17, 0x00, 0x80);
	
	// Reset CRTC1
	VIAVideo_int_ResetCRTC(0, TRUE);
	
	// -- Set framebuffer --
	// Set colour depth bits
	// VGASRMask(Crtc, 0x15, {0x00,0x14,0x0C}[Depth=8,16,24/32], 0x1C)
	switch(Depth) {
	case 8:  _SRMask(0x15, 0x00, 0x1C);	break;
	case 16: _SRMask(0x15, 0x14, 0x1C);	break;
	case 24: _SRMask(0x15, 0x0C, 0x1C);	break;
	case 32: _SRMask(0x15, 0x0C, 0x1C);	break;
	default:	return ;
	}
	// Set line length
	{
		 int	pitch = Timings->HDisplay * (Depth/8) / 8;
		// (Pitch/8) -> CR13 + CR35&0xE0
		_CRMask(0x13, pitch, 0xFF);
		_CRMask(0x35, (pitch>>8)<<5, 0xE0);
	}
	// - Set frame origin? (->FrameSet(Crtc, X, Y))
	// DWORD Offset in memory
	// Stored in CR0D + CR0C + CR34 + CR48&0x03

	// -- Set CRTC Mode --
	// NOTES:
	// - VGA Represents the signal cycle as Display, Blank with Sync anywhere in that
	// - Hence, I program the blanking as HBP, Sync, HFP (with the sync in the blanking area)

	// Set mode
	// VGAMiscMask(Crtc, (HSyncEnabled ? 0x40 : 0x00), 0x40);
	// VGAMiscMask(Crtc, (VSyncEnabled ? 0x80 : 0x00), 0x80);
	// HTotal:   CR00 + CR36&0x08 = /8-5
	temp = (Timings->HFP + Timings->HSync + Timings->HDisplay + Timings->HBP) / 8 - 5;
	_CRMask(0x00, temp, 0xFF);
	_CRMask(0x36, (temp>>8)<<3, 0x08);
	// HDisplay: CR01 = /8-1
	temp = Timings->HDisplay / 8 - 1;
	_CRMask(0x01, temp, 0xFF);
	// HBlank Start: CR02 = /8-1
	temp = (Timings->HDisplay) / 8 - 1;
	_CRMask(0x02, temp, 0xFF);
	// HBlank End:   CR03&0x1F + CR05&0x80 + CR33&0x20 = /8-1
	temp = (Timings->HBP + Timings->HSync + Timings->HFP) / 8 - 1;
	if( temp >> 7 )	Log_Error("VIA", "HBlank End doesn't fit (%i not 7 bits)", temp);
	_CRMask(0x03, temp, 0x1F);
	_CRMask(0x05, (temp>>5)<<7, 0x80);
	_CRMask(0x33, (temp>>6)<<5, 0x20);
	// HSync Start: CR04 + CR33&0x10 = /8
	temp = (Timings->HDisplay + Timings->HBP) / 8;
	_CRMask(0x04, temp, 0xFF);
	_CRMask(0x33, (temp>>8)<<4, 0x10);
	// HSync End:   CR05&0x1F = /8
	temp = (Timings->HSync) / 8;
	_CRMask(0x05, temp, 0x1F);
	// VTotal:   CR06 + CR07&0x01 + CR07&0x20 + CR35&0x01 = -2
	temp = (Timings->VFP + Timings->VBP + Timings->VSync + Timings->VDisplay) - 2;
	_CRMask(0x06, temp, 0xFF);
	_CRMask(0x07, (temp>>8)<<0, 0x01);
	_CRMask(0x07, (temp>>9)<<5, 0x20);
	_CRMask(0x35, (temp>>10)<<0, 0x01);
	// VDisplay: CR12 + CR07&0x02 + CR07&0x40 + CR35&0x04 = -1
	temp = (Timings->VDisplay) - 1;
	_CRMask(0x12, temp, 0xFF);
	_CRMask(0x07, (temp>>8)<<1, 0x02);
	_CRMask(0x07, (temp>>9)<<6, 0x40);
	_CRMask(0x07, (temp>>10)<<2, 0x04);
	// 0:  CR0C + CR0D + CD34 + CR48&0x03 ("Primary starting address")
	temp = 0;
	_CRMask(0x0C, temp, 0xFF);
	_CRMask(0x0D, (temp>>8), 0xFF);
	_CRMask(0x34, (temp>>16), 0xFF);
	_CRMask(0x48, (temp>>24), 0x03);
	// VSyncStart: CR10 + CR07&0x04 + CR07&0x80 + CR35&0x02
	temp = (Timings->VDisplay + Timings->HBP);
	_CRMask(0x10, temp, 0xFF);
	_CRMask(0x07, (temp>>8)<<2, 0x04);
	_CRMask(0x07, (temp>>9)<<7, 0x80);
	_CRMask(0x35, (temp>>10)<<1, 0x02);
	// VSyncEnd:   CR11&0x0F
	temp = (Timings->VSync);
	_CRMask(0x11, temp, 0x0F);
	// 0x3FFF: CR18 + CR07&0x10 + CR09&0x40 + CR33&0x06 + CR35&0x10 ("line compare")
	temp = 0x3FFF;
	_CRMask(0x18, temp, 0xFF);
	_CRMask(0x07, (temp>>8)<<4, 0x10);
	_CRMask(0x09, (temp>>9)<<6, 0x40);
	_CRMask(0x33, (temp>>10)<<1, 0x06);
	_CRMask(0x35, (temp>>12)<<4, 0x10);
	// 0:  CR09&0x1F ("Maximum scanline")
	temp = 0;
	_CRMask(0x09, temp, 0x1F);
	// 0: Cursor location
	temp = 0;
	_CRMask(0x14, temp, 0xFF);
	// VBlankStart: CR15 + CR07&0x08 + CR09&0x20 + CR35&0x08 = -1
	temp = (Timings->VDisplay) - 1;
	_CRMask(0x15, temp, 0xFF);
	_CRMask(0x07, (temp>>8)<<3, 0x08);
	_CRMask(0x09, (temp>>9)<<5, 0x20);
	_CRMask(0x35, (temp>>10)<<3, 0x08);
	// VBlankEnd:   CR16 = -1
	temp = (Timings->VBP + Timings->VSync + Timings->VFP) - 1;
	_CRMask(0x16, temp, 0xFF);
	// 0:  CR08 (Preset Row Scan Register)
	_CRMask(0x08, 0, 0xFF);
	// 0:  CR32 (???)
	_CRMask(0x32, 0, 0xFF);
	// 0:  CR33&0xC8
	_CRMask(0x33, 0, 0xC8);

	// Set scaling?

	// Some magic? (Not Simultaneous)
	_CRMask(0x6B, 0x00, 0x08);

	// Disable CRTC reset
	VIAVideo_int_ResetCRTC(0, FALSE);

	// Some other magic? (VGA - Lock registers?)
	_CRMask(0x17, 0x80, 0x80);
		
	_CRMask(0x36, 0x00, 0x30);	// CRT On (TODO: Others)
}

void VIAVideo_int_ResetCRTC(int Index, BOOL Reset)
{
	// VGASRMask(Crtc, 0x00, (Reset ? 0x00 : 0x02), 0x02);
	_SRMask(0x00, (Reset ? 0x00 : 0x02), 0x02);
}

void _SRMask(int Reg, Uint8 Byte, Uint8 Mask)
{
	Uint8	cv = 0;
	outb(0x3C4, Reg);
	if(Mask != 0xFF)	cv = inb(0x3C5) & ~Mask;
	cv |= Byte & Mask;
	outb(0x3C5, cv);
}

void _CRMask(int Reg, Uint8 Byte, Uint8 Mask)
{
	Uint8	cv = 0;
	outb(0x3D4, Reg);
	if(Mask != 0xFF)	cv = inb(0x3D5) & ~Mask;
	cv |= Byte & Mask;
	outb(0x3D5, cv);
}


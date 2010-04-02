/*
 * AcessOS 1
 * Video BIOS Extensions (Vesa) Driver
 */
#define DEBUG	0
#define VERSION	0x100

#include <acess.h>
#include <vfs.h>
#include <tpl_drv_video.h>
#include <fs_devfs.h>
#include <modules.h>
#include <vm8086.h>
#include "common.h"

// === CONSTANTS ===
#define	FLAG_LFB	0x1

// === PROTOTYPES ===
 int	Vesa_Install(char **Arguments);
Uint64	Vesa_Read(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer);
Uint64	Vesa_Write(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer);
 int	Vesa_Ioctl(tVFS_Node *Node, int ID, void *Data);
 int	Vesa_Int_SetMode(int Mode);
 int	Vesa_Int_FindMode(tVideo_IOCtl_Mode *data);
 int	Vesa_Int_ModeInfo(tVideo_IOCtl_Mode *data);

// === GLOBALS ===
MODULE_DEFINE(0, VERSION, Vesa, Vesa_Install, NULL, "PCI", "VM8086", NULL);
tDevFS_Driver	gVesa_DriverStruct = {
	NULL, "VESA",
	{
	.Read = Vesa_Read,
	.Write = Vesa_Write,
	.IOCtl = Vesa_Ioctl
	}
};
tSpinlock	glVesa_Lock;
tVM8086	*gpVesa_BiosState;
 int	giVesaCurrentMode = -1;
 int	giVesaDriverId = -1;
char	*gVesaFramebuffer = (void*)0xC00A0000;
tVesa_Mode	*gVesa_Modes;
 int	giVesaModeCount = 0;
 int	giVesaPageCount = 0;

//CODE
int Vesa_Install(char **Arguments)
{
	tVesa_CallInfo	*info;
	tFarPtr	infoPtr;
	tVesa_CallModeInfo	*modeinfo;
	tFarPtr	modeinfoPtr;
	Uint16	*modes;
	int	i;
	
	// Allocate Info Block
	gpVesa_BiosState = VM8086_Init();
	info = VM8086_Allocate(gpVesa_BiosState, 512, &infoPtr.seg, &infoPtr.ofs);
	modeinfo = VM8086_Allocate(gpVesa_BiosState, 512, &modeinfoPtr.seg, &modeinfoPtr.ofs);
	// Set Requested Version
	memcpy(info->signature, "VBE2", 4);
	// Set Registers
	gpVesa_BiosState->AX = 0x4F00;
	gpVesa_BiosState->ES = infoPtr.seg;	gpVesa_BiosState->DI = infoPtr.ofs;
	// Call Interrupt
	VM8086_Int(gpVesa_BiosState, 0x10);
	if(gpVesa_BiosState->AX != 0x004F) {
		Log_Warning("Vesa", "Vesa_Install - VESA/VBE Unsupported (AX = 0x%x)\n", gpVesa_BiosState->AX);
		return MODULE_ERR_NOTNEEDED;
	}
	
	modes = (Uint16 *) VM8086_GetPointer(gpVesa_BiosState, info->VideoModes.seg, info->VideoModes.ofs);
	
	// Read Modes
	for( giVesaModeCount = 1; modes[giVesaModeCount] != 0xFFFF; giVesaModeCount++ );
	gVesa_Modes = (tVesa_Mode *)malloc( giVesaModeCount * sizeof(tVesa_Mode) );
	
	// Insert Text Mode
	gVesa_Modes[0].width = 80;
	gVesa_Modes[0].height = 25;
	gVesa_Modes[0].bpp = 4;
	gVesa_Modes[0].code = 0x3;
	
	for( i = 1; i < giVesaModeCount; i++ )
	{
		gVesa_Modes[i].code = modes[i];
		// Get Mode info
		gpVesa_BiosState->AX = 0x4F01;
		gpVesa_BiosState->CX = gVesa_Modes[i].code;
		gpVesa_BiosState->ES = modeinfoPtr.seg;
		gpVesa_BiosState->DI = modeinfoPtr.ofs;
		VM8086_Int(gpVesa_BiosState, 0x10);
		
		// Parse Info
		gVesa_Modes[i].flags = 0;
		if ( (modeinfo->attributes & 0x90) == 0x90 )
		{
			gVesa_Modes[i].flags |= FLAG_LFB;
			gVesa_Modes[i].framebuffer = modeinfo->physbase;
			gVesa_Modes[i].fbSize = modeinfo->Xres*modeinfo->Yres*modeinfo->bpp/8;
		} else {
			gVesa_Modes[i].framebuffer = 0;
			gVesa_Modes[i].fbSize = 0;
		}
		
		gVesa_Modes[i].width = modeinfo->Xres;
		gVesa_Modes[i].height = modeinfo->Yres;
		gVesa_Modes[i].bpp = modeinfo->bpp;
		
		#if DEBUG
		LogF(" Vesa_Install: 0x%x - %ix%ix%i\n",
			gVesa_Modes[i].code, gVesa_Modes[i].width, gVesa_Modes[i].height, gVesa_Modes[i].bpp);
		#endif
	}
	
	
	// Install Device
	giVesaDriverId = DevFS_AddDevice( &gVesa_DriverStruct );
	if(giVesaDriverId == -1)	return MODULE_ERR_MISC;
	
	return MODULE_ERR_OK;
}

/* Read from the framebuffer
 */
Uint64 Vesa_Read(tVFS_Node *Node, Uint64 off, Uint64 len, void *buffer)
{
	#if DEBUG >= 2
	LogF("Vesa_Read: () - NULL\n");
	#endif
	return 0;
}

/**
 * \brief Write to the framebuffer
 */
Uint64 Vesa_Write(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer)
{
	ENTER("pNode XOffset XLength pBuffer", Node, Offset, Length, Bufffer);

	if(Buffer == NULL) {
		LEAVE('i', 0);
		return 0;
	}

	if( gVesa_Modes[giVesaCurrentMode].framebuffer == 0 ) {
		Log_Warning("VESA", "Vesa_Write - Non-LFB Modes not yet supported.");
		LEAVE('i', 0);
		return 0;
	}
	if(gVesa_Modes[giVesaCurrentMode].fbSize < Offset+Length)
	{
		Log_Warning("VESA", "Vesa_Write - Framebuffer Overflow");
		LEAVE('i', 0);
		return 0;
	}
	
	memcpy(gVesaFramebuffer + Offset, Buffer, Length);
	
	LEAVE('X', Length);
	return Length;
}

/**
 * \fn int Vesa_Ioctl(vfs_node *node, int id, void *data)
 * \brief Handle messages to the device
 */
int Vesa_Ioctl(tVFS_Node *Node, int ID, void *Data)
{
	switch(ID)
	{
	case DRV_IOCTL_TYPE:	return DRV_TYPE_VIDEO;
	case DRV_IOCTL_IDENT:	memcpy("VESA", Data, 4);	return 1;
	case DRV_IOCTL_VERSION:	return VERSION;
	
	case VIDEO_IOCTL_GETSETMODE:
		if( !Data )	return giVesaCurrentMode;
		return Vesa_Int_SetMode( *(int*)Data );
	
	case VIDEO_IOCTL_FINDMODE:
		return Vesa_Int_FindMode((tVideo_IOCtl_Mode*)Data);
	case VIDEO_IOCTL_MODEINFO:
		return Vesa_Int_ModeInfo((tVideo_IOCtl_Mode*)Data);
	
	case VIDEO_IOCTL_REQLFB:	// Request Linear Framebuffer
		return 0;
	}
	return 0;
}

int Vesa_Int_SetMode(int mode)
{	
	#if DEBUG
	LogF("Vesa_Int_SetMode: (mode=%i)\n", mode);
	#endif
	
	// Sanity Check values
	if(mode < 0 || mode > giVesaModeCount)	return -1;
	
	// Check for fast return
	if(mode == giVesaCurrentMode)	return 1;
	
	LOCK( &glVesa_Lock );
	
	gpVesa_BiosState->AX = 0x4F02;
	gpVesa_BiosState->BX = gVesa_Modes[mode].code;
	if(gVesa_Modes[mode].flags & FLAG_LFB) {
		LogF(" Vesa_Int_SetMode: Using LFB\n");
		gpVesa_BiosState->BX |= 0x4000;	// Bit 14 - Use LFB
	}
	
	// Set Mode
	VM8086_Int(gpVesa_BiosState, 0x10);
	
	// Map Framebuffer
	MM_UnmapHWPages((tVAddr)gVesaFramebuffer, giVesaPageCount);
	giVesaPageCount = (gVesa_Modes[mode].fbSize + 0xFFF) >> 12;
	gVesaFramebuffer = (void*)MM_MapHWPages(gVesa_Modes[mode].framebuffer, giVesaPageCount);
	
	LogF(" Vesa_Int_SetMode: Fb (Phys) = 0x%x\n", gVesa_Modes[mode].framebuffer);
	LogF(" Vesa_Int_SetMode: Fb (Virt) = 0x%x\n", gVesaFramebuffer);
	
	// Record Mode Set
	giVesaCurrentMode = mode;
	
	RELEASE( &glVesa_Lock );
	
	return 1;
}

int Vesa_Int_FindMode(tVideo_IOCtl_Mode *data)
{
	 int	i;
	 int	best = -1, bestFactor = 1000;
	 int	factor, tmp;
	#if DEBUG
	LogF("Vesa_Int_FindMode: (data={width:%i,height:%i,bpp:%i})\n", data->width, data->height, data->bpp);
	#endif
	for(i=0;i<giVesaModeCount;i++)
	{
		#if DEBUG >= 2
		LogF("Mode %i (%ix%ix%i), ", i, gVesa_Modes[i].width, gVesa_Modes[i].height, gVesa_Modes[i].bpp);
		#endif
	
		if(gVesa_Modes[i].width == data->width
		&& gVesa_Modes[i].height == data->height
		&& gVesa_Modes[i].bpp == data->bpp)
		{
			#if DEBUG >= 2
			LogF("Perfect!\n");
			#endif
			best = i;
			break;
		}
		
		tmp = gVesa_Modes[i].width * gVesa_Modes[i].height * gVesa_Modes[i].bpp;
		tmp -= data->width * data->height * data->bpp;
		tmp = tmp < 0 ? -tmp : tmp;
		factor = tmp * 100 / (data->width * data->height * data->bpp);
		
		#if DEBUG >= 2
		LogF("factor = %i\n", factor);
		#endif
		
		if(factor < bestFactor)
		{
			bestFactor = factor;
			best = i;
		}
	}
	data->id = best;
	data->width = gVesa_Modes[best].width;
	data->height = gVesa_Modes[best].height;
	data->bpp = gVesa_Modes[best].bpp;
	return best;
}

int Vesa_Int_ModeInfo(tVideo_IOCtl_Mode *data)
{
	if(data->id < 0 || data->id > giVesaModeCount)	return -1;
	data->width = gVesa_Modes[data->id].width;
	data->height = gVesa_Modes[data->id].height;
	data->bpp = gVesa_Modes[data->id].bpp;
	return 1;
}

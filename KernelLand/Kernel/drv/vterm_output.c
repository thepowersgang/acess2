/*
 * Acess2 Kernel
 * - By John Hodge (thePowersGang)
 *
 * drv/vterm_input.c
 * - Virtual Terminal - Input code
 */
#include "vterm.h"
#include <api_drv_video.h>
#define DEBUG	0

// === CODE ===
/**
 * \fn void VT_InitOutput()
 * \brief Initialise Video Output
 */
void VT_InitOutput()
{
	giVT_OutputDevHandle = VFS_Open(gsVT_OutputDevice, VFS_OPENFLAG_WRITE);
	if(giVT_OutputDevHandle == -1) {
		Log_Warning("VTerm", "Oh F**k, I can't open the video device '%s'", gsVT_OutputDevice);
		return ;
	}
	VT_SetResolution( giVT_RealWidth, giVT_RealHeight );
	VT_SetTerminal( 0 );
	VT_SetMode( VIDEO_BUFFMT_TEXT );
	LOG("VTerm output initialised");
}

/**
 * \brief Set video output buffer mode
 */
void VT_SetMode(int Mode)
{
	VFS_IOCtl( giVT_OutputDevHandle, VIDEO_IOCTL_SETBUFFORMAT, &Mode );
}

/**
 * \fn void VT_int_ScrollFramebuffer( tVTerm *Term, int Count )
 * \note Scrolls the framebuffer down by \a Count text lines
 */
void VT_int_ScrollFramebuffer( tVTerm *Term, int Count )
{
	 int	tmp;
	struct {
		Uint8	Op;
		Uint16	DstX, DstY;
		Uint16	SrcX, SrcY;
		Uint16	W, H;
	} PACKED	buf;
	
	// Only update if this is the current terminal
	if( Term != gpVT_CurTerm )	return;
	
	if( Count > Term->ScrollHeight )	Count = Term->ScrollHeight;
	if( Count < -Term->ScrollHeight )	Count = -Term->ScrollHeight;
	
	// Switch to 2D Command Stream
	tmp = VIDEO_BUFFMT_2DSTREAM;
	VFS_IOCtl(giVT_OutputDevHandle, VIDEO_IOCTL_SETBUFFORMAT, &tmp);
	
	// BLIT to 0,0 from 0,giVT_CharHeight
	buf.Op = VIDEO_2DOP_BLIT;
	buf.SrcX = 0;	buf.DstX = 0;
	// TODO: Don't assume character dimensions
	buf.W = Term->TextWidth * giVT_CharWidth;
	if( Count > 0 )
	{
		buf.SrcY = (Term->ScrollTop+Count) * giVT_CharHeight;
		buf.DstY = Term->ScrollTop * giVT_CharHeight;
	}
	else	// Scroll up, move text down
	{
		Count = -Count;
		buf.SrcY = Term->ScrollTop * giVT_CharHeight;
		buf.DstY = (Term->ScrollTop+Count) * giVT_CharHeight;
	}
	buf.H = (Term->ScrollHeight-Count) * giVT_CharHeight;
	VFS_WriteAt(giVT_OutputDevHandle, 0, sizeof(buf), &buf);
	
	// Restore old mode (this function is only called during text mode)
	tmp = VIDEO_BUFFMT_TEXT;
	VFS_IOCtl(giVT_OutputDevHandle, VIDEO_IOCTL_SETBUFFORMAT, &tmp);
}

void VT_int_UpdateCursor( tVTerm *Term, int bShow )
{
	tVideo_IOCtl_Pos	csr_pos;

	if( Term != gpVT_CurTerm )	return ;

	if( !bShow )
	{
		csr_pos.x = -1;	
		csr_pos.y = -1;	
	}
	else if( Term->Mode == TERM_MODE_TEXT )
	{
		 int	offset;
		
//		if( !(Term->Flags & VT_FLAG_SHOWCSR)
//		 && ( (Term->Flags & VT_FLAG_HIDECSR) || !Term->Node.ReadThreads)
//		  )
		if( !Term->Text || Term->Flags & VT_FLAG_HIDECSR )
		{
			csr_pos.x = -1;
			csr_pos.y = -1;
		}
		else
		{
			if(Term->Flags & VT_FLAG_ALTBUF)
				offset = Term->AltWritePos;
			else
				offset = Term->WritePos - Term->ViewPos;
					
			csr_pos.x = offset % Term->TextWidth;
			csr_pos.y = offset / Term->TextWidth;
			if( 0 > csr_pos.y || csr_pos.y >= Term->TextHeight )
				csr_pos.y = -1, csr_pos.x = -1;
		}
	}
	else
	{
		csr_pos.x = Term->VideoCursorX;
		csr_pos.y = Term->VideoCursorY;
	}
	VFS_IOCtl(giVT_OutputDevHandle, VIDEO_IOCTL_SETCURSOR, &csr_pos);
}	

/**
 * \fn void VT_int_UpdateScreen( tVTerm *Term, int UpdateAll )
 * \brief Updates the video framebuffer
 */
void VT_int_UpdateScreen( tVTerm *Term, int UpdateAll )
{
	// Only update if this is the current terminal
	if( Term != gpVT_CurTerm )	return;
	
	switch( Term->Mode )
	{
	case TERM_MODE_TEXT: {
		size_t view_pos = (Term->Flags & VT_FLAG_ALTBUF) ? 0 : Term->ViewPos;
		size_t write_pos = (Term->Flags & VT_FLAG_ALTBUF) ? Term->AltWritePos : Term->WritePos;
		const tVT_Char *buffer = (Term->Flags & VT_FLAG_ALTBUF) ? Term->AltBuf : Term->Text;
		// Re copy the entire screen?
		if(UpdateAll) {
			VFS_WriteAt(
				giVT_OutputDevHandle,
				0,
				Term->TextWidth*Term->TextHeight*sizeof(tVT_Char),
				&buffer[view_pos]
				);
		}
		// Only copy the current line
		else {
			 int	ofs = write_pos - write_pos % Term->TextWidth;
			VFS_WriteAt(
				giVT_OutputDevHandle,
				(ofs - view_pos)*sizeof(tVT_Char),
				Term->TextWidth*sizeof(tVT_Char),
				&buffer[ofs]
				);
		}
		break; }
	case TERM_MODE_FB:
		break;
	}
	
	VT_int_UpdateCursor(Term, 1);
}


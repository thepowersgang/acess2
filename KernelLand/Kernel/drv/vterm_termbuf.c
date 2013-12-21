/*
 * Acess2 Kernel
 * - By John Hodge (thePowersGang)
 *
 * drv/vterm_termbuf.c
 * - Virtual Terminal - Terminal buffer manipulation
 */
#define DEBUG	0
#include "vterm.h"

extern int	Term_HandleVT100(tVTerm *Term, int Len, const char *Buf);

// === CODE ===

/**
 * \fn void VT_int_PutString(tVTerm *Term, const Uint8 *Buffer, Uint Count)
 * \brief Print a string to the Virtual Terminal
 */
void VT_int_PutString(tVTerm *Term, const Uint8 *Buffer, Uint Count)
{
	// Iterate
	for( int ofs = 0; ofs < Count; )
	{
		int esc_len = Term_HandleVT100(Term, Count - ofs, (const void*)(Buffer + ofs));
		if( esc_len < 0 ) {
			esc_len = -esc_len;
			//LOG("%i '%*C'", esc_len, esc_len, Buffer+ofs);
			LOG("%i '%.*s'", esc_len, esc_len, Buffer+ofs);
			VT_int_PutRawString(Term, Buffer + ofs, esc_len);
			//Debug("Raw string '%.*s'", esc_len, Buffer+ofs);
		}
		else {
			//Debug("Escape code '%.*s'", esc_len, Buffer+ofs);
		}
		ASSERTCR(esc_len, >, 0, );
		ofs += esc_len;
	}
	// Update Screen
	VT_int_UpdateScreen( Term, 1 );
}

void VT_int_PutRawString(tVTerm *Term, const Uint8 *String, size_t Bytes)
{
	for( int i = 0; i < Bytes; ) {
		Uint32	val;
		i += ReadUTF8(String+i, &val);
		VT_int_PutChar(Term, val);
	}
}

/**
 * \fn void VT_int_PutChar(tVTerm *Term, Uint32 Ch)
 * \brief Write a single character to a VTerm
 */
void VT_int_PutChar(tVTerm *Term, Uint32 Ch)
{
	 int	i;
	tVT_Char	*buffer;
	 int	write_pos;
	 int	limit;
	
	if(Term->Flags & VT_FLAG_ALTBUF) {
		buffer = Term->AltBuf;
		write_pos = Term->AltWritePos;
		limit = Term->TextHeight * Term->TextWidth;
	}
	else {
		buffer = Term->Text;
		write_pos = Term->WritePos;
		limit = Term->TextHeight*(giVT_Scrollback+1) * Term->TextWidth;
	}
	
	switch(Ch)
	{
	case '\0':	return;	// Ignore NULL byte
	case '\n':
		VT_int_UpdateScreen( Term, 0 );	// Update the line before newlining
		write_pos += Term->TextWidth;
	case '\r':
		write_pos -= write_pos % Term->TextWidth;
		break;
	
	case '\t': {
		int line = write_pos / Term->TextWidth;
		write_pos %= Term->TextWidth;
		do {
			buffer[ write_pos ].Ch = '\0';
			buffer[ write_pos ].Colour = Term->CurColour;
			write_pos ++;
		} while(write_pos & 7);
		write_pos += line * Term->TextWidth;
		break; }
	
	case '\b':
		// Backspace is invalid at Offset 0
		if(write_pos == 0)	break;
		
		write_pos --;
		// Single Character
		if(buffer[ write_pos ].Ch != '\0') {
			buffer[ write_pos ].Ch = 0;
			buffer[ write_pos ].Colour = Term->CurColour;
			break;
		}
		// Tab
		i = 7;	// Limit it to 8
		do {
			buffer[ write_pos ].Ch = 0;
			buffer[ write_pos ].Colour = Term->CurColour;
			write_pos --;
		} while(write_pos && i-- && buffer[ write_pos ].Ch == '\0');
		if(buffer[ write_pos ].Ch != '\0')
			write_pos ++;
		break;
	
	default:
		ASSERTC(write_pos,<,limit);
		buffer[ write_pos ].Ch = Ch;
		buffer[ write_pos ].Colour = Term->CurColour;
		// Update the line before wrapping
		if( (write_pos + 1) % Term->TextWidth == 0 )
			VT_int_UpdateScreen( Term, 0 );
		write_pos ++;
		break;
	}
	
	if(Term->Flags & VT_FLAG_ALTBUF)
	{
		Term->AltWritePos = write_pos;
		
		if(Term->AltWritePos >= limit)
		{
			Term->AltWritePos -= Term->TextWidth;
			VT_int_ScrollText(Term, 1);
		}
	}
	else
	{
		Term->WritePos = write_pos;
		// Move Screen
		// - Check if we need to scroll the entire scrollback buffer
		if(Term->WritePos >= limit)
		{
			 int	base;
			
			// Update view position
			base = Term->TextWidth*Term->TextHeight*(giVT_Scrollback);
			if(Term->ViewPos < base)
				Term->ViewPos += Term->Width;
			if(Term->ViewPos > base)
				Term->ViewPos = base;
			
			VT_int_ScrollText(Term, 1);
			Term->WritePos -= Term->TextWidth;
		}
		// Ok, so we only need to scroll the screen
		else if(Term->WritePos >= Term->ViewPos + Term->TextWidth*Term->TextHeight)
		{
			VT_int_ScrollFramebuffer( Term, 1 );
			
			Term->ViewPos += Term->TextWidth;
		}
	}
	
	//LEAVE('-');
}

void VT_int_ScrollText(tVTerm *Term, int Count)
{
	tVT_Char	*buf;
	 int	height, init_write_pos;
	 int	len, i;
	 int	scroll_top, scroll_height;

	// Get buffer pointer and attributes	
	if( Term->Flags & VT_FLAG_ALTBUF )
	{
		buf = Term->AltBuf;
		height = Term->TextHeight;
		init_write_pos = Term->AltWritePos;
		scroll_top = Term->ScrollTop;
		scroll_height = Term->ScrollHeight;
	}
	else
	{
		buf = Term->Text;
		height = Term->TextHeight*(giVT_Scrollback+1);
		init_write_pos = Term->WritePos;
		scroll_top = 0;
		scroll_height = height;
	}

	// Scroll text upwards (more space at bottom)
	if( Count > 0 )
	{
		 int	base;
	
		// Set up
		if(Count > scroll_height)	Count = scroll_height;
		base = Term->TextWidth*(scroll_top + scroll_height - Count);
		len = Term->TextWidth*(scroll_height - Count);
		
		// Scroll terminal cache
		memmove(
			&buf[Term->TextWidth*scroll_top],
			&buf[Term->TextWidth*(scroll_top+Count)],
			len*sizeof(tVT_Char)
			);
		// Clear last rows
		for( i = 0; i < Term->TextWidth*Count; i ++ )
		{
			buf[ base + i ].Ch = 0;
			buf[ base + i ].Colour = Term->CurColour;
		}
		
		// Update Screen
		VT_int_ScrollFramebuffer( Term, Count );
		if( Term->Flags & VT_FLAG_ALTBUF )
			Term->AltWritePos = base;
		else
			Term->WritePos = Term->ViewPos + Term->TextWidth*(Term->TextHeight - Count);
		for( i = 0; i < Count; i ++ )
		{
			VT_int_UpdateScreen( Term, 0 );
			if( Term->Flags & VT_FLAG_ALTBUF )
				Term->AltWritePos += Term->TextWidth;
			else
				Term->WritePos += Term->TextWidth;
		}
	}
	else
	{
		Count = -Count;
		if(Count > scroll_height)	Count = scroll_height;
		
		len = Term->TextWidth*(scroll_height - Count);
		
		// Scroll terminal cache
		memmove(
			&buf[Term->TextWidth*(scroll_top+Count)],
			&buf[Term->TextWidth*scroll_top],
			len*sizeof(tVT_Char)
			);
		// Clear preceding rows
		for( i = 0; i < Term->TextWidth*Count; i ++ )
		{
			buf[ i ].Ch = 0;
			buf[ i ].Colour = Term->CurColour;
		}
		
		VT_int_ScrollFramebuffer( Term, -Count );
		if( Term->Flags & VT_FLAG_ALTBUF )
			Term->AltWritePos = Term->TextWidth*scroll_top;
		else
			Term->WritePos = Term->ViewPos;
		for( i = 0; i < Count; i ++ )
		{
			VT_int_UpdateScreen( Term, 0 );
			if( Term->Flags & VT_FLAG_ALTBUF )
				Term->AltWritePos += Term->TextWidth;
			else
				Term->WritePos += Term->TextWidth;
		}
	}
	
	if( Term->Flags & VT_FLAG_ALTBUF )
		Term->AltWritePos = init_write_pos;
	else
		Term->WritePos = init_write_pos;
}

/**
 * \brief Clears a line in a virtual terminal
 * \param Term	Terminal to modify
 * \param Num	Line number to clear
 */
void VT_int_ClearLine(tVTerm *Term, int Num)
{
	 int	limit = Term->TextHeight * (Term->Flags & VT_FLAG_ALTBUF ? 1 : giVT_Scrollback + 1);
	tVT_Char	*buffer = (Term->Flags & VT_FLAG_ALTBUF ? Term->AltBuf : Term->Text);
	if( Num < 0 || Num >= limit )	return ;
	
	tVT_Char	*cell = &buffer[ Num*Term->TextWidth ];
	
	for( int i = 0; i < Term->TextWidth; i ++ )
	{
		cell[ i ].Ch = 0;
		cell[ i ].Colour = Term->CurColour;
	}
}

/**
 * \brief Update the screen mode
 * \param Term	Terminal to update
 * \param NewMode	New mode to set
 * \param NewWidth	New framebuffer width
 * \param NewHeight	New framebuffer height
 */
void VT_int_Resize(tVTerm *Term, int NewWidth, int NewHeight)
{
	 int	oldW = Term->Width;
	 int	oldTW = Term->TextWidth;
	 int	oldH = Term->Height;
	 int	oldTH = Term->TextHeight;
	tVT_Char	*oldTBuf = Term->Text;
	Uint32	*oldFB = Term->Buffer;
	 int	w, h, i;
	
	// TODO: Increase RealWidth/RealHeight when this happens
	if(NewWidth > giVT_RealWidth)	NewWidth = giVT_RealWidth;
	if(NewHeight > giVT_RealHeight)	NewHeight = giVT_RealHeight;
	
	// Fast exit if no resolution change
	if(NewWidth == Term->Width && NewHeight == Term->Height)
		return ;
	
	// Calculate new dimensions
	Term->Width = NewWidth;
	Term->Height = NewHeight;
	Term->TextWidth = NewWidth / giVT_CharWidth;
	Term->TextHeight = NewHeight / giVT_CharHeight;
	Term->ScrollHeight = Term->TextHeight - (oldTH - Term->ScrollHeight) - Term->ScrollTop;
	
	// Allocate new buffers
	// - Text
	Term->Text = calloc(
		Term->TextWidth * Term->TextHeight * (giVT_Scrollback+1),
		sizeof(tVT_Char)
		);
	if(oldTBuf) {
		// Copy old buffer
		w = (oldTW > Term->TextWidth) ? Term->TextWidth : oldTW;
		h = (oldTH > Term->TextHeight) ? Term->TextHeight : oldTH;
		h *= giVT_Scrollback + 1;
		for( i = 0; i < h; i ++ )
		{
			memcpy(
				&Term->Text[i*Term->TextWidth],
				&oldTBuf[i*oldTW],
				w*sizeof(tVT_Char)
				);	
		}
		free(oldTBuf);
	}
	
	// - Alternate Text
	Term->AltBuf = realloc(
		Term->AltBuf,
		Term->TextWidth * Term->TextHeight * sizeof(tVT_Char)
		);
	
	// - Framebuffer
	if(oldFB) {
		Term->Buffer = calloc( Term->Width * Term->Height, sizeof(Uint32) );
		// Copy old buffer
		w = (oldW > Term->Width) ? Term->Width : oldW;
		h = (oldH > Term->Height) ? Term->Height : oldH;
		for( i = 0; i < h; i ++ )
		{
			memcpy(
				&Term->Buffer[i*Term->Width],
				&oldFB[i*oldW],
				w*sizeof(Uint32)
				);
		}
		free(oldFB);
	}
	
	// Debug
	#if 0
	switch(Term->Mode)
	{
	case TERM_MODE_TEXT:
		Log_Log("VTerm", "Set VT %p to text mode (%ix%i)",
			Term, Term->TextWidth, Term->TextHeight);
		break;
	case TERM_MODE_FB:
		Log_Log("VTerm", "Set VT %p to framebuffer mode (%ix%i)",
			Term, Term->Width, Term->Height);
		break;
	//case TERM_MODE_2DACCEL:
	//case TERM_MODE_3DACCEL:
	//	return;
	}
	#endif
}


void VT_int_ToggleAltBuffer(tVTerm *Term, int Enabled)
{	
	if(Enabled)
		Term->Flags |= VT_FLAG_ALTBUF;
	else
		Term->Flags &= ~VT_FLAG_ALTBUF;
	VT_int_UpdateScreen(Term, 1);
}

